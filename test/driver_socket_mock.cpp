// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#define NOMINMAX 
#include <iostream>
#include "lazy_sockets.h"
#include <errno.h>

#include <thread>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "timer.h"
#include "packets.h"


#define DriverLog MESSAGE

using namespace lsc;

using tcp_socket = LSocket<AF_INET, SOCK_STREAM, 0>;
using tcp_receiver_loop = ThreadedRecvLoop<AF_INET, SOCK_STREAM, 0, PacketEndTag>;

static constexpr int VRInitError_IPC_ServerInitFailed = -1;
static constexpr int VRInitError_IPC_ConnectFailed = -2;

class MockTrackingReference_hobovr {

public:
	MockTrackingReference_hobovr(std::string name) {
		DriverLog("tracking reference: Activate called");
	    m_pSocketComm = std::make_shared<tcp_socket>();

	    int res = m_pSocketComm->Connect("127.0.0.1", 6969);
        CHECK_FALSE_MESSAGE(res, "tracking reference: failed to connect: errno=", lerrno);

	    // send an id message saying this is a manager socket
	    res = m_pSocketComm->Send(KHoboVR_ManagerIdMessage, sizeof(KHoboVR_ManagerIdMessage));

        CHECK_FALSE_MESSAGE(res < 0, "tracking reference: failed to send id message: errno=", lerrno);

	    MESSAGE("tracking reference: manager socket fd=", (int)m_pSocketComm->GetHandle());

	    m_pReceiver = std::make_unique<tcp_receiver_loop>(
	        m_pSocketComm,
	        g_EndTag,
	        std::bind(&MockTrackingReference_hobovr::OnPacket, this, std::placeholders::_1, std::placeholders::_2)
	    );
	    m_pReceiver->Start();

	    DriverLog("tracking reference: receiver startup status: ", m_pReceiver->IsAlive());
	}

	void OnPacket(void* buff, size_t len) {
		// yeah dumb ass, it was this stupid of a fix
	    if (len != sizeof(HoboVR_ManagerMsg_t) - sizeof(PacketEndTag)) {
	        DriverLog("tracking reference: bad message");
	        HoboVR_ManagerResp_t resp{EManagerResp_invalid};
	        m_pSocketComm->Send(
	            &resp,
	            sizeof(resp)
	        );

	        return; // do nothing if bad message
	    }


	    DriverLog("tracking reference: got a packet lol");

	    HoboVR_ManagerMsg_t* message = (HoboVR_ManagerMsg_t*)buff;

	    if (message->type == EManagerMsgType_uduString) {
	    	std::string temp;
            for (int i=0; i < message->data.udu.len; i++) {
                if (message->data.udu.devices[i] == 0)
                    temp += "h";
                else if (message->data.udu.devices[i] == 1)
                    temp += "c";
                else if (message->data.udu.devices[i] == 2)
                    temp += "t";
                else if (message->data.udu.devices[i] == 3)
                    temp += "g";
            }
            _vpUduChangeBuffer = temp;

            // vr::VREvent_Notification_t event_data = {20, 0};
            // vr::VRServerDriverHost()->VendorSpecificEvent(
            //     m_unObjectId,
            //     vr::VREvent_VendorSpecific_HoboVR_UduChange,
            //     (vr::VREvent_Data_t&)event_data,
            //     0
            // );
            _UduEvent = true;

            DriverLog(
                "tracking reference: udu settings change request processed"
            );
            HoboVR_ManagerResp_t resp{EManagerResp_ok};
            m_pSocketComm->Send(
                &resp,
                sizeof(resp)
            );
	    }
	}

	bool GetUduEvent() {
	    if (!_UduEvent)
	        return false;

	    _UduEvent = false; // consume a true call
	    DriverLog("tracking reference: consume call triggered");
	    return true;
	}

	std::string GetUduBuffer() {
		return _vpUduChangeBuffer;
	}

private:
	// interproc sync
    bool _UduEvent;
    std::string _vpUduChangeBuffer; // for passing udu data

    std::shared_ptr<tcp_socket> m_pSocketComm;
    std::unique_ptr<tcp_receiver_loop> m_pReceiver;
};

using HobovrTrackingRef_SettManager = MockTrackingReference_hobovr;


namespace util {
	inline size_t udu2sizet(std::string udu_string) {
		size_t out = 0;
		for (auto i : udu_string) {
			switch(i) {
				case 'h': {out += sizeof(HoboVR_HeadsetPose_t); 		break;}
				case 'c': {out += sizeof(HoboVR_ControllerState_t); 	break;}
				case 't': {out += sizeof(HoboVR_TrackerPose_t); 		break;}
				case 'g': {out += sizeof(HoboVR_GazeState_t); 			break;}
			}
		}

		return out;
	}
}

class MockServerDriver_hobovr {
private:
	int bad_packet_count = 0;
public:
	MockServerDriver_hobovr() = default;

	int Init() {
		// time to init the connection to the poser :P
		mlSocket = std::make_shared<tcp_socket>();

		int res = mlSocket->Connect("127.0.0.1", 6969);
		if (res) {
			DriverLog("driver: failed to connect: errno=", lerrno);
			return VRInitError_IPC_ServerInitFailed;
		}

		DriverLog("driver: tracking socket fd=", (int)mlSocket->GetHandle());

		// send an id message saying this is a tracking socket
		res = mlSocket->Send(KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage));
		if (res < 0) {
	        DriverLog("driver: failed to send id message: errno=\n", lerrno);
	        return VRInitError_IPC_ConnectFailed;
	    }

		mpReceiver = std::make_unique<tcp_receiver_loop>(
			mlSocket,
			g_EndTag,
			std::bind(&MockServerDriver_hobovr::OnPacket, this, std::placeholders::_1, std::placeholders::_2),
			16
		);
		if (!mpReceiver) {
	        DriverLog("driver: failed to init receiver\n");
	        return -5;
		}
		mpReceiver->Start();

		// create manager now
		mpSettingsManager = std::make_unique<HobovrTrackingRef_SettManager>("trsm0");
		// misc start timer
		mpTimer = std::make_unique<hobovr::Timer>();
		// and add some timed events
		// equivalent of the old fast thread
		mpTimer->registerTimer(
			std::chrono::milliseconds(1),
			[this]() {
				// fast part
				// check of incoming udu events
				if (mpSettingsManager->GetUduEvent() && !mbUduEvent.load()) {
					DriverLog("udu change event");
					muInternalBufferSize = util::udu2sizet(
						mpSettingsManager->GetUduBuffer()
					);		

					// pause packet processing
					mbUduEvent.store(true);
					UpdateServerDeviceList();
					mpReceiver->ReallocInternalBuffer(muInternalBufferSize);
					// resume packet processing
					mbUduEvent.store(false);
					DriverLog("udu change event end");
				}
			}
		);

		return 0;
	}

	void Cleanup() {
		DriverLog("driver cleanup called");
		// set slow/fast thread alive signals to exit
		mpTimer.reset();

		// // send a "driver exit" notification to the poser
		uint32_t fake_data = 0;
		HoboVR_PoserResp_t resp{
			EPoserRespType_driverShutdown,
			(HoboVR_RespData_t&)fake_data,
			g_EndTag
		};
		mlSocket->Send(&resp, sizeof(resp));

		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		// release ownership to signal the receiver thread to finish
		mlSocket.reset();

		// call stop to make sure the receiver thread has joined
		mpReceiver->Stop();
	}

	void OnPacket(void* buff, size_t len) {
		if (len != muInternalBufferSize) {
			// tell the poser it fucked up
			// TODO: make different responses for different fuck ups...
			// 							and detect different fuck ups
			HoboVR_RespBufSize_t expected_size = {(uint32_t)muInternalBufferSize};

			HoboVR_PoserResp_t resp{
				EPoserRespType_badDeviceList,
				(HoboVR_RespData_t&)expected_size,
				g_EndTag
			};

			int res = mlSocket->Send(
				&resp,
				sizeof(resp)
			);
			// GOD FUCKING FINALLY

			// so logs in steamvr take ages to complete... TOO BAD!
			DriverLog("driver: posers are getting ignored~ expected ",
				(int)muInternalBufferSize,
				" bytes, got ",
				(int)len,
				" bytes~\n"
			);

			CHECK_FALSE_MESSAGE(bad_packet_count > 10, "too many packet miss matches");

			bad_packet_count++;

			return; // do nothing on bad messages
		}

		if (mbUduEvent.load()) {
			DriverLog("driver: encountered a packet on udu event");
			return; // sync in progress, do nothing
		}

		WARN_FALSE_MESSAGE(muInternalBufferSize != len,
			"packet missmatch, expected ",
			(int)muInternalBufferSize,
			" but got ",
			(int)len);

		bad_packet_count = 0;

		// DriverLog("received buffer ", (int)len);
	}

private:
	// void InternalThread();
	void UpdateServerDeviceList() {

	}

	size_t muInternalBufferSize = 16;

	std::shared_ptr<tcp_socket> mlSocket;
	std::unique_ptr<tcp_receiver_loop> mpReceiver;
	std::unique_ptr<hobovr::Timer> mpTimer;

	std::unique_ptr<HobovrTrackingRef_SettManager> mpSettingsManager;

	std::atomic<bool> mbUduEvent;
};

MockServerDriver_hobovr g_hobovrServerDriver;

TEST_CASE("Driver socket comm mock") {
	int res = g_hobovrServerDriver.Init();

	CHECK_FALSE_MESSAGE(res != 0, "bad startup status: %d", res);

	if (!res)
		std::this_thread::sleep_for(std::chrono::seconds(30));

	g_hobovrServerDriver.Cleanup();
}