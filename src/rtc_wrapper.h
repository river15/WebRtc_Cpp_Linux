//
// Created by webrtc on 6/2/22.
//

#ifndef SIMPLE_WEBRTC_CPP_LINUX_RTC_WRAPPER_H
#define SIMPLE_WEBRTC_CPP_LINUX_RTC_WRAPPER_H

#include "rtc_connection.h"
#include "rtc_ice.h"
#include "pc/video_track_source.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "test/vcm_capturer.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "file_process.h"

class CapturerTrackSource : public webrtc::VideoTrackSource {
public:
    static rtc::scoped_refptr<CapturerTrackSource> Create() {
        const size_t kWidth = 1280;
        const size_t kHeight = 720;
        const size_t kFps = 25;

        // device info
        char device_name[256];
        char unique_name[256];
        std::string local_device_name = lyx::config["device_name"];

        std::unique_ptr<webrtc::test::VcmCapturer> capturer;
        std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
                webrtc::VideoCaptureFactory::CreateDeviceInfo());

        if (!info) {
            return nullptr;
        }
        int num_devices = info->NumberOfDevices();

        if(local_device_name == "only_one"){
            for (int i = 0; i < num_devices; ++i) {
                capturer = absl::WrapUnique(
                        capturer->Create(kWidth, kHeight,  kFps, i));
                if (capturer) {
                    return new rtc::RefCountedObject<CapturerTrackSource>(
                            std::move(capturer));
                }
            }
        }else{
            for (int i = 0; i < num_devices; ++i) {
                info->GetDeviceName(i,device_name, sizeof(device_name), unique_name,
                                    sizeof(unique_name));
                std::string str_device_name = device_name;
                int find_state = str_device_name.find(local_device_name);
                if (find_state >= 0 && find_state < 30){
                    capturer = absl::WrapUnique(
                            capturer->Create(kWidth, kHeight,  kFps, i));
                    if (capturer) {
                        return new rtc::RefCountedObject<CapturerTrackSource>(
                                std::move(capturer));
                    }
                }else{
                    std::cout << "no found device;" << std::endl;
                    return nullptr;
                }
            }
        }


        return nullptr;
    }
protected:
    explicit CapturerTrackSource(
            std::unique_ptr<webrtc::test::VcmCapturer> capturer)
            : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer))  {}
private:
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
        return capturer_.get();
    }
    std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};


class Wrapper {
public:
    const std::string name;
    std::unique_ptr<rtc::Thread> network_thread;
    std::unique_ptr<rtc::Thread> worker_thread;
    std::unique_ptr<rtc::Thread> signaling_thread;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
    webrtc::PeerConnectionInterface::RTCConfiguration configuration;
    Connection connection;

    Wrapper(const std::string name_) : name(name_), connection(name_) {
    }

    void on_sdp(std::function<void(const std::string &)> f) {
        connection.on_sdp = f;
    }

    void on_accept_ice(std::function<void()> f) {
        connection.on_accept_ice = f;
    }

    void on_ice(std::function<void(const Ice &)> f) {
        connection.on_ice = f;
    }

    void on_success(std::function<void()> f) {
        connection.on_success = f;
    }

    void on_message(std::function<void(const std::string &)> f) {
        connection.on_message = f;
    }

    void init() {
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "Main thread has initiated..." << std::endl;

        // Using Google's STUN server.
        webrtc::PeerConnectionInterface::IceServer ice_server;
        ice_server.uri = "stun:stun.l.google.com:19302";
        configuration.servers.push_back(ice_server);
        configuration.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

//        configuration.allow_codec_switching= false;
//        configuration.sdp_semantics = webrtc::SdpSemantics::kPlanB;



        network_thread = rtc::Thread::CreateWithSocketServer();
        network_thread->Start();
        worker_thread = rtc::Thread::Create();
        worker_thread->Start();
        signaling_thread = rtc::Thread::Create();
        signaling_thread->Start();
        webrtc::PeerConnectionFactoryDependencies dependencies;
        dependencies.network_thread   = network_thread.get();
        dependencies.worker_thread    = worker_thread.get();
        dependencies.signaling_thread = signaling_thread.get();

        //peer_connection_factory       = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));

        peer_connection_factory = webrtc::CreatePeerConnectionFactory(
                network_thread.get() /* network_thread */, worker_thread.get() /* worker_thread */,
                signaling_thread.get() /* signaling_thread */, nullptr /* default_adm */,
                webrtc::CreateBuiltinAudioEncoderFactory(),
                webrtc::CreateBuiltinAudioDecoderFactory(),
                webrtc::CreateBuiltinVideoEncoderFactory(),
                webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
                nullptr /* audio_processing */);

        if (peer_connection_factory.get() == nullptr) {
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Error on CreateModularPeerConnectionFactory." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void create_offer_sdp() {
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "create_offer_sdp" << std::endl;

        connection.peer_connection =
                peer_connection_factory->CreatePeerConnection(configuration, nullptr, nullptr, &connection.pco);
//        webrtc::PeerConnectionInterface::BitrateParameters bp;
//        bp.current_bitrate_bps = 30000000;
//        bp.max_bitrate_bps = 30000000;
//        bp.min_bitrate_bps = 30000000;
//        connection.peer_connection->SetBitrate(bp);


        if (connection.peer_connection.get() == nullptr) {
            peer_connection_factory = nullptr;
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Error on CreatePeerConnection." << std::endl;
            exit(EXIT_FAILURE);
        }

        //start
        std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
                connection.peer_connection->GetSenders();
        if (!senders.empty()) {
            std::cout << "Already added tracks." << std::endl;
            return;  // Already added tracks.
        }

        std::string stream_suffix = lyx::config["srs_webrtc_streamurl_suffix"]; // kliveStream


        // Add Audio Track
        rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
                peer_connection_factory->CreateAudioTrack(
                        "kAudioLabel", peer_connection_factory->CreateAudioSource(
                                cricket::AudioOptions())));
        audio_track->set_enabled(true);
        auto result_or_error_audio =  connection.peer_connection->AddTrack(audio_track, {stream_suffix});


        if (!result_or_error_audio.ok()) {
            RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                              << result_or_error_audio.error().message();
        }

        // Add Video Track
        rtc::scoped_refptr<CapturerTrackSource> video_device = CapturerTrackSource::Create();
        if (video_device) {
            rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
                    peer_connection_factory->CreateVideoTrack("kVideoLabel", video_device));
            video_track_->set_enabled(true);

            // simulcast
            auto result_or_error_video = connection.peer_connection->AddTrack(video_track_, {stream_suffix});
            if (!result_or_error_video.ok()) {
                RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                                  << result_or_error_video.error().message();
            }
        } else {
            RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
        }

        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options =
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
//        options.num_simulcast_layers = 3;
        // end

        connection.peer_connection->CreateOffer(connection.csdo, options);
    }

    void create_answer_sdp(const std::string &parameter) {
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "create_answer_sdp" << std::endl;

        connection.peer_connection =
                peer_connection_factory->CreatePeerConnection(configuration, nullptr, nullptr, &connection.pco);

        if (connection.peer_connection.get() == nullptr) {
            peer_connection_factory = nullptr;
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Error on CreatePeerConnection." << std::endl;
            exit(EXIT_FAILURE);
        }
        webrtc::SdpParseError error;
        webrtc::SessionDescriptionInterface *session_description(
                webrtc::CreateSessionDescription("offer", parameter, &error));
        if (session_description == nullptr) {
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Error on CreateSessionDescription." << std::endl
                      << error.line << std::endl
                      << error.description << std::endl;
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Offer SDP:begin" << std::endl
                      << parameter << std::endl
                      << "Offer SDP:end" << std::endl;
            exit(EXIT_FAILURE);
        }
        connection.peer_connection->SetRemoteDescription(connection.ssdo, session_description);
        connection.peer_connection->CreateAnswer(connection.csdo, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }

    void push_reply_sdp(const std::string &parameter) {
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "push_reply_sdp" << std::endl;

        webrtc::SdpParseError error;
        webrtc::SessionDescriptionInterface *session_description(
                webrtc::CreateSessionDescription("answer", parameter, &error));
        if (session_description == nullptr) {
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Error on CreateSessionDescription." << std::endl
                      << error.line << std::endl
                      << error.description << std::endl;
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Answer SDP:begin" << std::endl
                      << parameter << std::endl
                      << "Answer SDP:end" << std::endl;
            exit(EXIT_FAILURE);
        }
        connection.peer_connection->SetRemoteDescription(connection.ssdo, session_description);
    }

    void push_ice(const Ice &ice_it) {
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "push_ice" << std::endl;

        webrtc::SdpParseError err_sdp;
        webrtc::IceCandidateInterface *ice =
                CreateIceCandidate(ice_it.sdp_mid, ice_it.sdp_mline_index, ice_it.candidate, &err_sdp);
        if (!err_sdp.line.empty() && !err_sdp.description.empty()) {
            std::cout << name << ":" << std::this_thread::get_id() << ":"
                      << "Error on CreateIceCandidate" << std::endl
                      << err_sdp.line << std::endl
                      << err_sdp.description << std::endl;
            exit(EXIT_FAILURE);
        }
        connection.peer_connection->AddIceCandidate(ice);
    }

    void send(const std::string &parameter) {
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "send" << std::endl;

        webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(parameter.c_str(), parameter.size()), true);
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "Send(" << connection.data_channel->state() << ")" << std::endl;
        connection.data_channel->Send(buffer);
    }

    void quit() {
        std::cout << name << ":" << std::this_thread::get_id() << ":"
                  << "quit" << std::endl;

        // Close with the thread running.
        connection.peer_connection->Close();
        connection.peer_connection = nullptr;
        connection.data_channel    = nullptr;
        peer_connection_factory    = nullptr;

        network_thread->Stop();
        worker_thread->Stop();
        signaling_thread->Stop();
    }
};



#endif //SIMPLE_WEBRTC_CPP_LINUX_RTC_WRAPPER_H
