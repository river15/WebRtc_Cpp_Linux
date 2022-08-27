//
// Created by webrtc on 6/2/22.
//

#ifndef SIMPLE_WEBRTC_CPP_LINUX_RTC_CONNECTION_H
#define SIMPLE_WEBRTC_CPP_LINUX_RTC_CONNECTION_H

#include "rtc_ice.h"
#include "json.hpp"
#include "http.h"
#include "file_process.h"

class Connection {
public:
    const std::string name;

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;

    std::function<void(const std::string &)> on_sdp;
    std::function<void()> on_accept_ice;
    std::function<void(const Ice &)> on_ice;
    std::function<void()> on_success;
    std::function<void(const std::string &)> on_message;

    // When the status of the DataChannel changes, determine if the connection is complete.
    void on_state_change() {
        std::cout << "state: " << data_channel->state() << std::endl;
        if (data_channel->state() == webrtc::DataChannelInterface::kOpen && on_success) {
            on_success();
        }
    }

    // After the SDP is successfully created, it is set as a LocalDescription and displayed as a string to be passed to
    // the other party.
    void on_success_csd(webrtc::SessionDescriptionInterface *desc) {
        peer_connection->SetLocalDescription(ssdo, desc);
        std::cout << "on_success_csd coming " << std::endl;
        //SRS 4.0 only support h.264 in sdp
        std::string sdpOffer;
        desc->ToString(&sdpOffer);

        std::string srs_api = lyx::config["srs_push_addr"]; // http://121.37.25.119:1985/rtc/v1/publish?numberOfSimulcastLayers=spec3
        std::string srs_streamurl = lyx::config["srs_webrtc_streamurl"]; // webrtc://121.37.25.119/live/kliveStream?numberOfSimulcastLayers=spec3

        std::cout << "sdp offer : " <<  sdpOffer << std::endl;

        //send offer sdp to srs4.0
        //json使用nlohmann::json
        nlohmann::json reqMsg = {
                {"api", srs_api},
//                {"tid", "wsy-R"},
//                {"clientip", "null"},
                {"streamurl",  srs_streamurl},
                {"sdp", sdpOffer}
        };

        std::string url = srs_api;
        std::string response;
        std::string httpRequestBody = reqMsg.dump();
        std::cout << "httpRequestBody: " << httpRequestBody << std::endl;


        int http_request_state = httpClient.post(url, nullptr,httpRequestBody, &response);
        std::cout << "http_request_state : " <<  http_request_state << std::endl;
        if (http_request_state == 0){
            std::cout << "http response : " <<  response << std::endl;
            nlohmann::json jsonContent = nlohmann::json::parse(response);


            int srs_response_code;
            std::string sdp;
            sdp = jsonContent["sdp"];
            std::cout << "sdp answer : " <<  sdp << std::endl;
            srs_response_code = (int)jsonContent["code"];
            if (srs_response_code == 0){
                webrtc::SdpParseError error;
                webrtc::SdpType type = webrtc::SdpType::kAnswer;

                std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
                        webrtc::CreateSessionDescription(type, sdp, &error);

                peer_connection->SetRemoteDescription(
                        DummySetSessionDescriptionObserver::Create(),
                        session_description.release());
            }else{
                std::cout << "SRS response has problem,sdp is " <<  sdp << std::endl;
            }

        }else{
            std::cout << "Request SRS failed,httpState is " <<  http_request_state << std::endl;
        }

    }

    // Convert the got ICE.
    void on_ice_candidate(const webrtc::IceCandidateInterface *candidate) {
        Ice ice;
        candidate->ToString(&ice.candidate);
        ice.sdp_mid         = candidate->sdp_mid();
        ice.sdp_mline_index = candidate->sdp_mline_index();
        on_ice(ice);
    }

    class PCO : public webrtc::PeerConnectionObserver {
    private:
        Connection &parent;

    public:
        PCO(Connection &parent) : parent(parent) {
        }

        void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::SignalingChange(" << new_state << ")" << std::endl;
        };

        void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::AddStream" << std::endl;
        };

        void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::RemoveStream" << std::endl;
        };

        void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::DataChannel(" << data_channel << ", " << parent.data_channel.get() << ")"
                      << std::endl;
            // The request recipient gets a DataChannel instance in the onDataChannel event.
            parent.data_channel = data_channel;
            parent.data_channel->RegisterObserver(&parent.dco);
        };

        void OnRenegotiationNeeded() override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::RenegotiationNeeded" << std::endl;
        };

        void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::IceConnectionChange(" << new_state << ")" << std::endl;
        };

        void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::IceGatheringChange(" << new_state << ")" << std::endl;
        };

        void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "PeerConnectionObserver::IceCandidate" << std::endl;
            parent.on_ice_candidate(candidate);
        };
    };

    class DCO : public webrtc::DataChannelObserver {
    private:
        Connection &parent;

    public:
        DCO(Connection &parent) : parent(parent) {
        }

        void OnStateChange() override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "DataChannelObserver::StateChange" << std::endl;
            parent.on_state_change();
        };

        // Message receipt.
        void OnMessage(const webrtc::DataBuffer &buffer) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "DataChannelObserver::Message" << std::endl;
            if (parent.on_message) {
                parent.on_message(std::string(buffer.data.data<char>(), buffer.data.size()));
            }
        };

        void OnBufferedAmountChange(uint64_t previous_amount) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "DataChannelObserver::BufferedAmountChange(" << previous_amount << ")" << std::endl;
        };
    };

    class CSDO : public webrtc::CreateSessionDescriptionObserver {
    private:
        Connection &parent;

    public:
        CSDO(Connection &parent) : parent(parent) {
        }

        void OnSuccess(webrtc::SessionDescriptionInterface *desc) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "CreateSessionDescriptionObserver::OnSuccess" << std::endl;
            parent.on_success_csd(desc);
        };

        void OnFailure(webrtc::RTCError error) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "CreateSessionDescriptionObserver::OnFailure" << std::endl
                      << error.message() << std::endl;
        };
    };

    class SSDO : public webrtc::SetSessionDescriptionObserver {
    private:
        Connection &parent;

    public:
        SSDO(Connection &parent) : parent(parent) {
        }

        void OnSuccess() override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "SetSessionDescriptionObserver::OnSuccess" << std::endl;
            if (parent.on_accept_ice) {
                parent.on_accept_ice();
            }
        };

        void OnFailure(webrtc::RTCError error) override {
            std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                      << "SetSessionDescriptionObserver::OnFailure" << std::endl
                      << error.message() << std::endl;
        };
    };

    class DummySetSessionDescriptionObserver
            : public webrtc::SetSessionDescriptionObserver {
    public:
        static DummySetSessionDescriptionObserver* Create() {
            return new  rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
        }
        virtual void OnSuccess() {
            std::cout << "##########OnSuccess";
            RTC_LOG(INFO) << __FUNCTION__;
        }
        virtual void OnFailure(webrtc::RTCError error) {
            std::cout << "##########OnFailure";
            RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ":  " << error.message();
        }
    };

    PCO pco;
    DCO dco;
    rtc::scoped_refptr<CSDO> csdo;
    rtc::scoped_refptr<SSDO> ssdo;
    lyx::HttpClient httpClient;

    Connection(const std::string &name_) :
            name(name_),
            pco(*this),
            dco(*this),
            csdo(new rtc::RefCountedObject<CSDO>(*this)),
            ssdo(new rtc::RefCountedObject<SSDO>(*this)) {
    }
};




#endif //SIMPLE_WEBRTC_CPP_LINUX_RTC_CONNECTION_H
