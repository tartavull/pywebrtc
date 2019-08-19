/*
 * webrtc has to be compiled with -fno-rtti while pybind11 
 * requires rtti to work.
 * The not so satisfactory solution we found is to create 
 * two dynamic libraries:
 * 1) libwebrtc.so is compiled with -fno-rtti and interacts with
 * the native webrtc.
 * 2) pybind.so is compiled with rtti and is the module imported
 * by the python interpreter. It is a thin wrapper around libwebrtc.
 */

#include "libwebrtc.h"

#include <iostream>
#include <string>
#include <thread>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

namespace py = pybind11;
py::object offer_future;
py::object answer_future;
PyThreadState *gil;
PyGILState_STATE gstate;
std::function<void(std::string,std::string)> python_logging;

void guarded_log(std::string level, std::string msg) {
    gstate = PyGILState_Ensure();
    python_logging(level, msg);
    PyGILState_Release(gstate);
}

void set_logging_callback(std::function<void(std::string,std::string)> fn) {
    python_logging = fn;
}

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
    pywebrtc::set_log_function(guarded_log);
    pywebrtc::initialize();
    pywebrtc::create_offer(offer_callback);
    PyEval_RestoreThread(gil);
}

void create_answer(std::string offer, py::object future) {
    gil = PyEval_SaveThread();
    answer_future = future;
    pywebrtc::set_log_function(guarded_log);
    pywebrtc::initialize();
    pywebrtc::create_answer(offer, answer_callback);
    PyEval_RestoreThread(gil);
}

void set_answer(const std::string& answer) {
    gil = PyEval_SaveThread();
    pywebrtc::set_answer(answer);
    PyEval_RestoreThread(gil);
}

auto cleanup_callback = []() {
    std::cout << "module destructor called" << std::endl;
    pywebrtc::destructor();
};


PYBIND11_MODULE(pybind, m) {
    m.def("set_logging_callback", &set_logging_callback);
    m.def("create_offer", &create_offer);
    m.def("create_answer", &create_answer);
    m.def("set_answer", &set_answer);
    m.def("get_candidates", &pywebrtc::get_candidates);
    m.def("set_candidates", &pywebrtc::set_candidates);
    m.def("send_data", &pywebrtc::send_data);
    m.add_object("_cleanup", py::capsule(cleanup_callback));
}
