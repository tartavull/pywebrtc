#include "libwebrtc.h"

#include <iostream>
#include <string>
#include <thread>
#include <pybind11/pybind11.h>

namespace py = pybind11;
py::object offer_future;
py::object answer_future;
PyThreadState *gil;
PyGILState_STATE gstate;

void offer_callback(std::string offer) {
    /* This is being called from a thread which is not the main, read
     * https://docs.python.org/3/c-api/init.html#non-python-created-threads
     */
    gstate = PyGILState_Ensure();
    offer_future.attr("set_result")(offer);
    PyGILState_Release(gstate);
}

void answer_callback(std::string answer) {
    /* This is being called from a thread which is not the main, read
     * https://docs.python.org/3/c-api/init.html#non-python-created-threads
     */
    gstate = PyGILState_Ensure();
    answer_future.attr("set_result")(answer);
    PyGILState_Release(gstate);
}

void create_offer(py::object future) {
    /*
     * We want this thread to release the gil so that python code keeps
     * executing.
     */
    gil = PyEval_SaveThread();
    offer_future = future;
    pywebrtc::initialize();
    pywebrtc::create_offer(offer_callback);
    PyEval_RestoreThread(gil);
}

void create_answer(std::string offer, py::object future) {
    gil = PyEval_SaveThread();
    answer_future = future;
    pywebrtc::initialize();
    pywebrtc::create_answer(offer, answer_callback);
    PyEval_RestoreThread(gil);
}

void set_answer(const std::string& answer) {
    gil = PyEval_SaveThread();
    pywebrtc::set_answer(answer);
    PyEval_RestoreThread(gil);
}

std::string get_candidates() {
    return pywebrtc::get_candidates();
}

void set_candidates(const std::string& candidates) {
    pywebrtc::set_candidates(candidates);
}

void send_data(const std::string& data) {
    pywebrtc::send_data(data);
}

auto cleanup_callback = []() {
    pywebrtc::destructor();
};


PYBIND11_MODULE(pybind, m) {
    m.def("create_offer", &create_offer);
    m.def("create_answer", &create_answer);
    m.def("set_answer", &set_answer);
    m.def("get_candidates", &get_candidates);
    m.def("set_candidates", &set_candidates);
    m.def("send_data", &send_data);
    m.add_object("_cleanup", py::capsule(cleanup_callback));
}
