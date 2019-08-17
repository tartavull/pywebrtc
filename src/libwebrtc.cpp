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

class Connection {
 public:
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
  std::string sdp_type;
  picojson::array ice_array;

  // Offer/Answerの作成が成功したら、LocalDescriptionとして設定 & 相手に渡す文字列として表示
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
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::SignalingChange(" << new_state << ")" << std::endl;
    };

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::AddStream" << std::endl;
    };

    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::RemoveStream" << std::endl;
    };

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::DataChannel(" << data_channel
                << ", " << parent.data_channel.get() << ")" << std::endl;
      // Answer送信側は、onDataChannelでDataChannelの接続を受け付ける
      parent.data_channel = data_channel;
      parent.data_channel->RegisterObserver(&parent.dco);
    };

    void OnRenegotiationNeeded() override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::RenegotiationNeeded" << std::endl;
    };

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceConnectionChange(" << new_state << ")" << std::endl;
    };

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceGatheringChange(" << new_state << ")" << std::endl;
    };

    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceCandidate" << std::endl;
      parent.onIceCandidate(candidate);
    };
  };
  
  class DCO : public webrtc::DataChannelObserver {
   private:
    Connection& parent;

   public:
    DCO(Connection& parent) : parent(parent) {
    }

    // 接続状況が変化した時に発火する。切断は発火タイミングで値を確認して検知可能
    void OnStateChange() override {
      std::cout << std::this_thread::get_id() << ":"
                << "DataChannelObserver::StateChange" << std::endl;
    };
    
    // メッセージ受信
    void OnMessage(const webrtc::DataBuffer& buffer) override {
      std::cout << std::this_thread::get_id() << ":"
                << "DataChannelObserver::Message" << std::endl;
      std::cout << std::string(buffer.data.data<char>(), buffer.data.size()) << std::endl;
    };

    void OnBufferedAmountChange(uint64_t previous_amount) override {
      std::cout << std::this_thread::get_id() << ":"
                << "DataChannelObserver::BufferedAmountChange(" << previous_amount << ")" << std::endl;
    };
  };

  class CSDO : public webrtc::CreateSessionDescriptionObserver {
   private:
    Connection& parent;

   public:
    CSDO(Connection& parent) : parent(parent) {
    }
  
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
      std::cout << std::this_thread::get_id() << ":"
                << "CreateSessionDescriptionObserver::OnSuccess" << std::endl;
      parent.onSuccessCSD(desc);
    };

    void OnFailure(const std::string& error) override {
      std::cout << std::this_thread::get_id() << ":"
                << "CreateSessionDescriptionObserver::OnFailure" << std::endl << error << std::endl;
    };
  };

  class SSDO : public webrtc::SetSessionDescriptionObserver {
   private:
    Connection& parent;

   public:
    SSDO(Connection& parent) : parent(parent) {
    }
    
    void OnSuccess() override {
      std::cout << std::this_thread::get_id() << ":"
                << "SetSessionDescriptionObserver::OnSuccess" << std::endl;
    };

    void OnFailure(const std::string& error) override {
      std::cout << std::this_thread::get_id() << ":"
                << "SetSessionDescriptionObserver::OnFailure" << std::endl << error << std::endl;
    };
  };

  PCO  pco;
  DCO  dco;
  rtc::scoped_refptr<CSDO> csdo;
  rtc::scoped_refptr<SSDO> ssdo;

  Connection() :
      pco(*this),
      dco(*this),
      csdo(new rtc::RefCountedObject<CSDO>(*this)),
      ssdo(new rtc::RefCountedObject<SSDO>(*this)) {
  }
};

std::unique_ptr<rtc::Thread> network_thread;
std::unique_ptr<rtc::Thread> worker_thread;
std::unique_ptr<rtc::Thread> signaling_thread;
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
    std::cout << "Error on CreatePeerConnection." << std::endl;
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
    std::cout << "Error on CreatePeerConnection." << std::endl;
    return;
  }
  webrtc::SdpParseError error;
  webrtc::SessionDescriptionInterface* session_description(
      webrtc::CreateSessionDescription("offer", offer, &error));
  if (session_description == nullptr) {
    std::cout << "Error on CreateSessionDescription." << std::endl
              << error.line << std::endl
              << error.description << std::endl;
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
    std::cout << "Error on CreateSessionDescription." << std::endl
              << error.line << std::endl
              << error.description << std::endl;
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
