#include <functional>
#include <string>

namespace pywebrtc
{
    int initialize();
    void destructor();
    void set_logging(const std::function<void(std::string, std::string)> fn);
    void set_on_message(const std::function<void(std::string)> fn);
    void create_offer(std::function<void(std::string)> callback);
    void create_answer(const std::string& offer,  std::function<void(std::string)> callback);
    void set_answer(const std::string& answer);
    std::string get_candidates();
    void set_candidates(const std::string& candidates);
    void send_data(const std::string& data);
}

