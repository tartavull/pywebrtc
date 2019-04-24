#include <iostream>
#include <thread>

#include <api/peerconnectioninterface.h>

class DCO : public webrtc::DataChannelObserver {
    public:
        DCO() {
        }

        void OnStateChange() override {
            std::cout << std::this_thread::get_id() << ":"
                    << "DataChannelObserver::StateChange" << std::endl;
        };
        
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

class Connection {
    public:
      rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
      rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
      std::string sdp_type;

      void onSuccessCSD(webrtc::SessionDescriptionInterface* desc) {
        peer_connection->SetLocalDescription(ssdo, desc);
        std::string sdp;
        desc->ToString(&sdp);
        std::cout << sdp_type << " SDP:begin" << std::endl << sdp << sdp_type << " SDP:end" << std::endl;
      }

      void onIceCandidate(const webrtc::IceCandidateInterface* candidate) {
        std::string candidate_str;
        candidate->ToString(&candidate_str);

        //picojson::object ice;
        //ice.insert(std::make_pair("candidate", picojson::value(candidate_str)));
        //ice.insert(std::make_pair("sdpMid", picojson::value(candidate->sdp_mid())));
        //ice.insert(std::make_pair("sdpMLineIndex", picojson::value(static_cast<double>(candidate->sdp_mline_index()))));
        //ice_array.push_back(picojson::value(ice));
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
          dco(),
          csdo(new rtc::RefCountedObject<CSDO>(*this)),
          ssdo(new rtc::RefCountedObject<SSDO>(*this)) {
      }
};


