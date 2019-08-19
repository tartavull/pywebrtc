#include "libwebrtc.h"

#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>


#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/peerconnectioninterface.h>
#include <rtc_base/flags.h>
#include <rtc_base/physicalsocketserver.h>
#include <rtc_base/ssladapter.h>
#include <rtc_base/thread.h>
#include <rtc_base/logging.h>

#include "picojson/picojson.h"

namespace pywebrtc
{

std::function<void(std::string)> offer_callback;
std::function<void(std::string)> answer_callback;
std::function<void(std::string,std::string)> python_log;
std::function<void(std::string)> python_on_message;

void set_logging(const std::function<void(std::string, std::string)> fn) {
    python_log = fn;
}
void set_on_message(const std::function<void(std::string)> fn) {
    python_on_message = fn;
}

class Connection {
 public:
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
  std::string sdp_type;
  picojson::array ice_array;

  void onSuccessCSD(webrtc::SessionDescriptionInterface* desc) {
    peer_connection->SetLocalDescription(ssdo, desc);
    std::string sdp;
    desc->ToString(&sdp);
    if (sdp_type == "Offer") {
        offer_callback(sdp);
    } else {
        answer_callback(sdp);
    }
  }

  // ICEを取得したら、表示用JSON配列の末尾に追加
  void onIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    picojson::object ice;
    std::string candidate_str;
    candidate->ToString(&candidate_str);
    ice.insert(std::make_pair("candidate", picojson::value(candidate_str)));
    ice.insert(std::make_pair("sdpMid", picojson::value(candidate->sdp_mid())));
    ice.insert(std::make_pair("sdpMLineIndex", picojson::value(static_cast<double>(candidate->sdp_mline_index()))));
    ice_array.push_back(picojson::value(ice));
  }

  class PCO : public webrtc::PeerConnectionObserver {
   private:
    Connection& parent;

   public:
    PCO(Connection& parent) : parent(parent) {
    }
  
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
      python_log("debug" , "PeerConnectionObserver::SignalingChange("+std::to_string(new_state)+")");
    };

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
      python_log("debug" , "PeerConnectionObserver::AddStream");
    };

    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
      python_log("debug" , "PeerConnectionObserver::RemoveStream");
    };

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::DataChannel(" << data_channel
                << ", " << parent.data_channel.get() << ")" << std::endl;
      parent.data_channel = data_channel;
      parent.data_channel->RegisterObserver(&parent.dco);
    };

    void OnRenegotiationNeeded() override {
      python_log("debug" , "PeerConnectionObserver::OnRenegotiationNeeded");
    };

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
        // I have no idea why, but uncommenting python_log make the program freeze
        //python_log("debug" , "PeerConnectionObserver::IceConnectionChange");
    };

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
        python_log("debug" , "PeerConnectionObserver::IceGatheringChange");
    };

    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
      python_log("debug" , "PeerConnectionObserver::IceCandidate");
      parent.onIceCandidate(candidate);
    };
  };
  
  class DCO : public webrtc::DataChannelObserver {
   public:
    void OnStateChange() override {
        python_log("debug" , "DataChannelObserver::StateChange");
    };
    
    void OnMessage(const webrtc::DataBuffer& buffer) override {
        python_on_message(std::string(buffer.data.data<char>(), buffer.data.size()));
    };

    void OnBufferedAmountChange(uint64_t previous_amount) override {
        python_log("debug" , "DataChannelObserver::BufferedAmountChange");
    };
  };

  class CSDO : public webrtc::CreateSessionDescriptionObserver {
   private:
    Connection& parent;

   public:
    CSDO(Connection& parent) : parent(parent) {
    }
  
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
      python_log("debug" , "CreateSessionDescriptionObserver::OnSuccess");
      parent.onSuccessCSD(desc);
    };

    void OnFailure(const std::string& error) override {
      python_log("warn" , "CreateSessionDescriptionObserver::OnFailure\n"+ error);
    };
  };

  class SSDO : public webrtc::SetSessionDescriptionObserver {
   public:
    void OnSuccess() override {
      python_log("debug" , "SetSessionDescriptionObserver::OnSuccess");
    };

    void OnFailure(const std::string& error) override {
      python_log("warn" , "SetSessionDescriptionObserver::OnFailure\n"+ error);
    };
  };

  PCO  pco;
  DCO  dco;
  rtc::scoped_refptr<CSDO> csdo;
  rtc::scoped_refptr<SSDO> ssdo;

  Connection() :
      pco(*this),
      dco(),
      csdo(new rtc::RefCountedObject<CSDO>(*this)),
      ssdo(new rtc::RefCountedObject<SSDO>()) {
  }
};

auto network_thread = std::make_unique<rtc::Thread>();
auto worker_thread = std::make_unique<rtc::Thread>();
auto signaling_thread = std::make_unique<rtc::Thread>();
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
webrtc::PeerConnectionInterface::RTCConfiguration configuration;
Connection connection;

void create_offer(std::function<void(std::string)> callback) {
  offer_callback = callback;
  connection.peer_connection = peer_connection_factory->CreatePeerConnection(
              configuration, nullptr, nullptr, &connection.pco);

  webrtc::DataChannelInit config;
  connection.data_channel = connection.peer_connection->CreateDataChannel("data_channel", &config);
  connection.data_channel->RegisterObserver(&connection.dco);

  if (connection.peer_connection.get() == nullptr) {
    peer_connection_factory = nullptr;
    python_log("warn", "Error on CreatePeerConnection.");
    return;
  }
  connection.sdp_type = "Offer";  
  connection.peer_connection->CreateOffer(
          connection.csdo, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void create_answer(const std::string& offer, std::function<void(std::string)> callback) {
  answer_callback = callback;
  connection.peer_connection =
      peer_connection_factory->CreatePeerConnection(configuration, nullptr,
              nullptr, &connection.pco);

  if (connection.peer_connection.get() == nullptr) {
    peer_connection_factory = nullptr;
    python_log("warn", "Error on CreatePeerConnection.");
    return;
  }
  webrtc::SdpParseError error;
  webrtc::SessionDescriptionInterface* session_description(
      webrtc::CreateSessionDescription("offer", offer, &error));
  if (session_description == nullptr) {
    python_log("warn", "Error on CreateSessionDescription.\n" + error.line + "\n" +  error.description);
  }
  connection.peer_connection->SetRemoteDescription(connection.ssdo, session_description);

  connection.sdp_type = "Answer"; // 表示用の文字列、webrtcの動作には関係ない
  connection.peer_connection->CreateAnswer(connection.csdo,
                                           webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void set_answer(const std::string& answer) {
  webrtc::SdpParseError error;
  webrtc::SessionDescriptionInterface* session_description(
      webrtc::CreateSessionDescription("answer", answer, &error));
  if (session_description == nullptr) {
    python_log("warn", "Error on CreateSessionDescription.\n" + error.line + "\n" +  error.description);
  }
  connection.peer_connection->SetRemoteDescription(
          connection.ssdo, session_description);
}

std::string get_candidates() {
  auto candidates = picojson::value(connection.ice_array).serialize(true);
  connection.ice_array.clear();
  return candidates;
}

void set_candidates(const std::string& candidates) {
  picojson::value v;
  std::string err = picojson::parse(v, candidates);
  if (!err.empty()) {
    std::cout << "Error on parse json : " << err << std::endl;
    return;
  }

  webrtc::SdpParseError err_sdp;
  for (auto& ice_it : v.get<picojson::array>()) {
    picojson::object& ice_json = ice_it.get<picojson::object>();
    webrtc::IceCandidateInterface* ice =
      CreateIceCandidate(ice_json.at("sdpMid").get<std::string>(),
                         static_cast<int>(ice_json.at("sdpMLineIndex").get<double>()),
                         ice_json.at("candidate").get<std::string>(),
                         &err_sdp);
    if (!err_sdp.line.empty() && !err_sdp.description.empty()) {
      std::cout << "Error on CreateIceCandidate" << std::endl
                << err_sdp.line << std::endl
                << err_sdp.description << std::endl;
      return;
    }
    connection.peer_connection->AddIceCandidate(ice);
  }
}

void send_data(const std::string& data) {
  webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(data.c_str(), data.size()), true);
  connection.data_channel->Send(buffer);
}

void destructor() {
  rtc::CleanupSSL();
  connection.peer_connection->Close();
  connection.peer_connection = nullptr;
  connection.data_channel = nullptr;
  peer_connection_factory = nullptr;
  network_thread->Stop();
  worker_thread->Stop();
  signaling_thread->Stop();
}

int initialize () {
  rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
    std::cout << std::this_thread::get_id() << ":"
            << "Main thread" << std::endl;

  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = "stun:stun.l.google.com:19302";
  configuration.servers.push_back(ice_server);

  rtc::InitializeSSL();
  network_thread = rtc::Thread::CreateWithSocketServer();
  network_thread->Start();
  worker_thread = rtc::Thread::Create();
  worker_thread->Start();
  signaling_thread = rtc::Thread::Create();
  signaling_thread->Start();

  peer_connection_factory = webrtc::CreatePeerConnectionFactory(
      network_thread.get(),
      worker_thread.get(),
      signaling_thread.get(),
      nullptr ,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(),
      nullptr ,
      nullptr );

  if (peer_connection_factory.get() == nullptr) {
    std::cout << "Error on CreatePeerConnectionFactory." << std::endl;
    return EXIT_FAILURE;
  }
}
}
