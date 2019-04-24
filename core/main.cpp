#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <exception>

#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/peer_connection_interface.h>
#include <api/create_peerconnection_factory.h>
#include <rtc_base/flags.h>
#include <rtc_base/physicalsocketserver.h>
#include <rtc_base/ssladapter.h>
#include <rtc_base/thread.h>

#include <boost/python.hpp>
#include "Python.h"
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/exception_translator.hpp>

#include "connection.cpp"

std::unique_ptr<rtc::Thread> network_thread;
std::unique_ptr<rtc::Thread> worker_thread;
std::unique_ptr<rtc::Thread> signaling_thread;
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
webrtc::PeerConnectionInterface::RTCConfiguration configuration;
Connection connection;


void initialize() {
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
      nullptr /* default_adm */,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      nullptr, //webrtc::CreateBuiltinVideoEncoderFactory(),
      nullptr, //webrtc::CreateBuiltinVideoDecoderFactory(),
      nullptr /* audio_mixer */,
      nullptr /* audio_processing */);

  if (peer_connection_factory.get() == nullptr) {
    std::cout << "Error on CreatePeerConnectionFactory." << std::endl;
  }
    std::cout << "Successfully initialized" << std::endl;
}

void  create_offer() {
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
  connection.peer_connection->CreateOffer(connection.csdo,
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}


struct OutOfSteakException {};

void translateException( const OutOfSteakException& x) {
    PyErr_SetString(PyExc_UserWarning, "The meat is gone, go for the cheese....");
};

char const* greet()
{
   return "hello, world";
}

BOOST_PYTHON_MODULE(core)
{
    using namespace boost::python;
    register_exception_translator<OutOfSteakException>(translateException);

    def("greet", greet);
    def("initialize", initialize);
    def("create_offer", create_offer);
}
