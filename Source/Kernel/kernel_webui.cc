// Ikaros 3.0

#include "ikaros.h"

#include <future>
#include <sstream>

using namespace ikaros;
using namespace std::chrono;

namespace ikaros
{
    namespace
    {
        constexpr char ui_subscription_separator = '\n';
        constexpr double ui_subscription_timeout_seconds = 10.0;
        constexpr size_t default_max_retained_webui_log_messages = 500;
        constexpr int ui_snapshot_rgb_jpeg_quality = 75;
        constexpr int ui_snapshot_gray_jpeg_quality = 70;

        bool is_snapshot_image_format(const std::string & format)
        {
            return format == "rgb" || format == "gray" || format == "red" ||
                   format == "green" || format == "blue" || format == "spectrum" ||
                   format == "fire";
        }

        int snapshot_jpeg_quality_for_format(const std::string & format)
        {
            return format == "rgb" ? ui_snapshot_rgb_jpeg_quality :
                                     ui_snapshot_gray_jpeg_quality;
        }
    }


    std::string
    Kernel::DoSendDataStatus()
    {
        std::ostringstream response;
        std::string nm;
        if(info_.contains_non_null("filename"))
            nm = std::string(info_["filename"]);
        else
            nm = options_.stem();

        if(!nm.empty())
            nm = std::filesystem::path(nm).filename().string();

        response << "\t\"file\": " << value(nm).json() << ",\n";

#if DEBUG
        response << "\t\"debug\": true,\n";
#else
        response << "\t\"debug\": false,\n";
#endif

        response << "\t\"state\": " << run_mode.load() << ",\n";
        if(stop_after != -1)
        {
            response << "\t\"tick\": \"" << tick << " / " << stop_after << "\",\n";
            response << "\t\"progress\": "
                     << (stop_after > 0 ? static_cast<double>(tick) / static_cast<double>(stop_after) : 0.0)
                     << ",\n";
        }
        else
        {
            response << "\t\"progress\": 0,\n";
        }

        // Timing information

        double uptime = uptime_timer.GetTime();
        double total_time = GetTime();

        response << "\t\"timestamp\": " << GetTimeStamp() << ",\n";
        response << "\t\"uptime\": " << uptime << ",\n";
        response << "\t\"tick_duration\": " << tick_duration << ",\n";
        response << "\t\"webui_req_int\": " << WebUIRequestInterval() << ",\n";
        response << "\t\"cpu_cores\": " << cpu_cores << ",\n";
    
        switch(run_mode)
        {
            case run_mode_stop:
                response << "\t\"tick\": \"-\",\n";
                response << "\t\"time\": \"-\",\n";
                response << "\t\"ticks_per_s\": \"-\",\n";
                response << "\t\"actual_duration\": \"-\",\n";
                response << "\t\"lag\": \"-\",\n";
                response << "\t\"time_usage\": 0,\n";
                response << "\t\"cpu_usage\": 0,\n";
                break;

            case run_mode_pause:
                response << "\t\"tick\": " << GetTick() << ",\n";
                response << "\t\"time\": " << GetTime() << ",\n";
                response << "\t\"ticks_per_s\": \"-\",\n";
                response << "\t\"actual_duration\": \"-\",\n";
                response << "\t\"lag\": \"-\",\n";
                response << "\t\"time_usage\": " << (actual_tick_duration> 0 ? tick_time_usage/actual_tick_duration : 0) << ",\n";
                response << "\t\"cpu_usage\": " << cpu_usage << ",\n";
                break;

            case run_mode_realtime:
            default:
                response << "\t\"tick\": " << GetTick() << ",\n";
                response << "\t\"time\": " << GetTime() << ",\n";
                response << "\t\"ticks_per_s\": " << (tick>0 ? double(tick)/total_time: 0) << ",\n";
                response << "\t\"actual_duration\": " << actual_tick_duration << ",\n";
                response << "\t\"lag\": " << lag << ",\n";
                response << "\t\"time_usage\": " << (actual_tick_duration> 0 ? tick_time_usage/actual_tick_duration : 0) << ",\n";
                response << "\t\"cpu_usage\": " << cpu_usage << ",\n";
                break;
        }

        response << "\t\"async\": {";
        std::string sep;
        for(auto & [path, component] : components)
        {
            if(!component->async_mode)
                continue;

            response << sep
                     << "\"" << escape_json_string(path) << "\": {"
                     << "\"running\": " << (component->IsAsyncRunning() ? "true" : "false") << ", "
                     << "\"failed\": " << (component->IsAsyncFailed() ? "true" : "false") << ", "
                     << "\"pending\": " << (component->IsAsyncPending() ? "true" : "false") << ", "
                     << "\"started_tick\": " << component->async_started_tick.load() << ", "
                     << "\"completed_tick\": " << component->async_completed_tick.load()
                     << "}";
            sep = ", ";
        }
        response << "},\n";

        return response.str();
    }


    std::string
    Kernel::NormalizeUIRoot(const std::string & component_path) const
    {
        std::string root = component_path;
        if(!root.empty() && root[0] == '.')
            root = root.substr(1);
        return root;
    }


    const parameter *
    Kernel::FindTopGroupParameter(const std::string & name) const
    {
        if(top_group_path.empty())
            return nullptr;

        auto it = parameters.find(top_group_path + "." + name);
        return it == parameters.end() ? nullptr : &it->second;
    }


    double
    Kernel::WebUIRequestInterval() const
    {
        if(const parameter * request_interval = FindTopGroupParameter("webui_req_int"))
        {
            try
            {
                return std::max(0.001, request_interval->as_double());
            }
            catch(const std::exception &)
            {
            }
        }
        return 0.1;
    }


    double
    Kernel::SnapshotInterval() const
    {
        if(const parameter * snapshot_interval = FindTopGroupParameter("snapshot_interval"))
        {
            try
            {
                return std::max(0.0, snapshot_interval->as_double());
            }
            catch(const std::exception &)
            {
            }
        }
        return 0.1;
    }


    size_t
    Kernel::MaxRetainedWebUILogMessages() const
    {
        if(const parameter * buffer_limit = FindTopGroupParameter("webui_log_buffer_limit"))
        {
            try
            {
                return static_cast<size_t>(std::max(1, buffer_limit->as_int()));
            }
            catch(const std::exception &)
            {
            }
        }
        return default_max_retained_webui_log_messages;
    }


    int
    Kernel::SnapshotJPEGQualityForFormat(const std::string & format) const
    {
        const char * parameter_name = format == "rgb" ? "rgb_quality" : "gray_quality";
        if(const parameter * quality_parameter = FindTopGroupParameter(parameter_name))
        {
            try
            {
                int quality = quality_parameter->as_int();
                return std::clamp(quality, 1, 100);
            }
            catch(const std::exception &)
            {
            }
        }
        return snapshot_jpeg_quality_for_format(format);
    }


    std::vector<Kernel::RequestedUIValue>
    Kernel::ParseRequestedUIValues(Request & request)
    {
        std::vector<RequestedUIValue> requested_values;
        std::string data;
        if(request.parameters.contains("data"))
            data = std::string(request.parameters["data"]);

        std::string root = NormalizeUIRoot(request.component_path);
        while(!data.empty())
        {
            std::string token = head(data, ",");
            if(token.empty())
                continue;

            std::string source = token;
            std::string format = rtail(source, ":");

            requested_values.push_back({
                root,
                token,
                token,
                source,
                format
            });
        }

        return requested_values;
    }


    Kernel::RequestedUIValue
    Kernel::ParseSubscribedUIValue(const std::string & subscription_key) const
    {
        RequestedUIValue requested_value;
        auto separator = subscription_key.find(ui_subscription_separator);
        if(separator == std::string::npos)
            requested_value.token = subscription_key;
        else
        {
            requested_value.root = subscription_key.substr(0, separator);
            requested_value.token = subscription_key.substr(separator + 1);
        }

        requested_value.key = requested_value.token;
        requested_value.source = requested_value.token;
        requested_value.format = rtail(requested_value.source, ":");
        return requested_value;
    }


    std::string
    Kernel::SubscriptionKeyFor(const RequestedUIValue & requested_value) const
    {
        return requested_value.root + ui_subscription_separator + requested_value.token;
    }


    bool
    Kernel::SerializeRequestedValue(RequestedUIValue requested_value, std::string & serialized_value, long long * compute_us, long long * value_us)
    {
        auto value_start = steady_clock::now();
        if((requested_value.source.find('@') != std::string::npos || requested_value.source.find('{') != std::string::npos) && components.count(requested_value.root) > 0)
        {
            auto compute_start = steady_clock::now();
            Component * component = components[requested_value.root].get();
            requested_value.source = component->ComputeValue(requested_value.source);
            if(compute_us)
                *compute_us += duration_cast<microseconds>(steady_clock::now() - compute_start).count();
        }

        std::string source_with_root = requested_value.root + "." + requested_value.source;
        if(!requested_value.source.empty() && requested_value.source[0] == '.')
            source_with_root = requested_value.source.substr(1);

        std::string component_path = peek_rhead(source_with_root, ".");
        std::string attribute = peek_rtail(source_with_root, ".");

        bool found_value = false;
        if(buffers.count(source_with_root) && !state_buffers.count(source_with_root))
        {
            if(ValueOwnedByRunningAsyncComponent(source_with_root))
                return false;

            if(requested_value.format.empty())
                serialized_value = buffers[source_with_root].json();
            else if(requested_value.format == "metadata")
                serialized_value = buffers[source_with_root].metadata_json();
            else if(is_snapshot_image_format(requested_value.format))
                serialized_value = SendImage(buffers[source_with_root], requested_value.format, SnapshotJPEGQualityForFormat(requested_value.format));
            found_value = !serialized_value.empty();
        }
        else if(parameters.count(source_with_root))
        {
            parameter & parameter_value = parameters[source_with_root];
            if(requested_value.format == "metadata" && parameter_value.get_type() == matrix_type)
            {
                const matrix & matrix_value = parameter_value.matrix_ref();
                serialized_value = matrix_value.metadata_json();
            }
            else
                serialized_value = parameter_value.json();
            found_value = true;
        }
        else if(components.count(component_path))
        {
            if(components[component_path]->IsAsyncRunning())
                return false;
            serialized_value = components[component_path]->json(attribute);
            found_value = !serialized_value.empty();
        }

        if(value_us)
            *value_us += duration_cast<microseconds>(steady_clock::now() - value_start).count();
        return found_value;
    }


    std::string
    Kernel::ConsumeLogForClient(long ui_client_id)
    {
        std::string response = ",\n\"log\": [";
        std::string sep;
        std::lock_guard<std::mutex> client_lock(ui_client_mutex);
        std::lock_guard<std::mutex> log_lock(log_mutex);

        auto & client_state = ui_client_states[ui_client_id];
        const uint64_t latest_sequence = next_webui_log_sequence - 1;

        if(!client_state.log_delivery_initialized)
        {
            client_state.delivered_log_sequence = first_webui_log_sequence - 1;
            client_state.log_delivery_initialized = true;
        }

        uint64_t next_sequence = client_state.delivered_log_sequence + 1;
        if(next_sequence < first_webui_log_sequence && next_sequence <= latest_sequence)
        {
            const uint64_t dropped_count = first_webui_log_sequence - next_sequence;
            const std::string dropped_message =
                "WebUI log truncated. Dropped " + std::to_string(dropped_count) +
                " older log message" + (dropped_count == 1 ? "" : "s") + " for this client.";
            response += Message(msg_warning, dropped_message).json();
            sep = ",";
            next_sequence = first_webui_log_sequence;
        }

        for(uint64_t sequence = next_sequence; sequence <= latest_sequence; ++sequence)
        {
            const size_t index = static_cast<size_t>(sequence - first_webui_log_sequence);
            response += sep + log[index].json();
            sep = ",";
        }

        client_state.delivered_log_sequence =
            std::max(client_state.delivered_log_sequence, latest_sequence);
        response += "]";
        return response;
    }

    void
    Kernel::ResetUISnapshotCache()
    {
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            current_ui_snapshot.reset();
        }
        {
            const auto now = steady_clock::now();
            std::lock_guard<std::mutex> lock(ui_client_mutex);
            for(auto & client_entry : ui_client_states)
            {
                auto & client_state = client_entry.second;
                client_state.keys.clear();
                client_state.last_seen_time = now;
            }
            ++ui_subscription_revision;
        }
    }


    Kernel::UISnapshotBuildPlan
    Kernel::PlanUISnapshotBuild(bool respect_rate_limit)
    {
        UISnapshotBuildPlan plan;
        plan.now = steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            plan.previous_snapshot = current_ui_snapshot;
        }

        plan.snapshot_due = !respect_rate_limit || plan.previous_snapshot == nullptr;
        {
            std::lock_guard<std::mutex> lock(ui_client_mutex);
            bool removed_client = false;
            for(auto it = ui_client_states.begin(); it != ui_client_states.end();)
            {
                if(plan.now - it->second.last_seen_time > duration<double>(ui_subscription_timeout_seconds))
                {
                    it = ui_client_states.erase(it);
                    removed_client = true;
                }
                else
                    ++it;
            }

            if(removed_client)
                ++ui_subscription_revision;

            plan.has_active_clients = !ui_client_states.empty();
            plan.subscription_revision = ui_subscription_revision;
            const bool subscriptions_changed = plan.previous_snapshot == nullptr ||
                plan.previous_snapshot->subscription_revision != plan.subscription_revision;
            if(subscriptions_changed)
                plan.snapshot_due = true;
            else if(!plan.snapshot_due)
                plan.snapshot_due = plan.now - plan.previous_snapshot->timestamp >=
                    duration<double>(WebUIRequestInterval());

            if(plan.snapshot_due)
                for(const auto & client_entry : ui_client_states)
                    plan.subscriptions.insert(client_entry.second.keys.begin(), client_entry.second.keys.end());
        }

        return plan;
    }


    void
    Kernel::PopulateUISnapshot(UISnapshot & snapshot, const UISnapshotBuildPlan & plan)
    {
        const bool refresh_images = plan.previous_snapshot == nullptr ||
            plan.now - plan.previous_snapshot->image_timestamp >= duration<double>(SnapshotInterval());
        snapshot.snapshot_id = next_ui_snapshot_id++;
        snapshot.subscription_revision = plan.subscription_revision;
        snapshot.session_id = session_id;
        snapshot.tick = tick;
        snapshot.image_timestamp = refresh_images ? plan.now : plan.previous_snapshot->image_timestamp;
        snapshot.status_json = DoSendDataStatus();

        std::vector<std::future<std::pair<std::string, std::string>>> image_futures;
        for(const auto & subscription_key : plan.subscriptions)
        {
            RequestedUIValue requested_value = ParseSubscribedUIValue(subscription_key);
            if(is_snapshot_image_format(requested_value.format))
            {
                if(!refresh_images)
                {
                    auto it = plan.previous_snapshot->serialized_values.find(subscription_key);
                    if(it != plan.previous_snapshot->serialized_values.end())
                    {
                        snapshot.serialized_values[subscription_key] = it->second;
                        continue;
                    }
                }

                image_futures.push_back(std::async(std::launch::async, [this, subscription_key, requested_value, previous_snapshot = plan.previous_snapshot]() mutable
                {
                    std::string serialized_value;
                    if(SerializeRequestedValue(requested_value, serialized_value))
                        return std::make_pair(subscription_key, std::move(serialized_value));
                    if(previous_snapshot != nullptr)
                    {
                        auto it = previous_snapshot->serialized_values.find(subscription_key);
                        if(it != previous_snapshot->serialized_values.end())
                            return std::make_pair(subscription_key, it->second);
                    }
                    return std::make_pair(std::string(), std::string());
                }));
            }
            else
            {
                try
                {
                    std::string serialized_value;
                    if(SerializeRequestedValue(requested_value, serialized_value))
                        snapshot.serialized_values[subscription_key] = std::move(serialized_value);
                    else if(plan.previous_snapshot != nullptr)
                    {
                        auto it = plan.previous_snapshot->serialized_values.find(subscription_key);
                        if(it != plan.previous_snapshot->serialized_values.end())
                            snapshot.serialized_values[subscription_key] = it->second;
                    }
                }
                catch(const std::exception & e)
                {
                    Notify(msg_warning, "Could not build UI snapshot for \"" + requested_value.token + "\": " + std::string(e.what()));
                }
            }
        }

        for(auto & future : image_futures)
        {
            try
            {
                auto result = future.get();
                if(!result.first.empty())
                    snapshot.serialized_values[result.first] = std::move(result.second);
            }
            catch(const std::exception & e)
            {
                Notify(msg_warning, "Could not build UI image snapshot: " + std::string(e.what()));
            }
        }

        snapshot.timestamp = steady_clock::now();
    }


    void
    Kernel::PublishUISnapshot(std::shared_ptr<UISnapshot> snapshot)
    {
        std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
        current_ui_snapshot = std::move(snapshot);
    }


    void
    Kernel::BuildUISnapshot(bool respect_rate_limit)
    {
        UISnapshotBuildPlan plan = PlanUISnapshotBuild(respect_rate_limit);
        if(!plan.has_active_clients)
        {
            PublishUISnapshot(nullptr);
            return;
        }
        if(!plan.snapshot_due)
            return;

        auto snapshot = std::make_shared<UISnapshot>();
        PopulateUISnapshot(*snapshot, plan);
        PublishUISnapshot(std::move(snapshot));
    }


    std::string
    Kernel::DoSendLog(Request & request)
    {
        return ConsumeLogForClient(request.client_id);
    }


    bool
    Kernel::UpdateUIClientSubscriptions(long client_id,
                                        const std::vector<RequestedUIValue> & requested_values)
    {
        std::unordered_set<std::string> requested_subscriptions;
        requested_subscriptions.reserve(requested_values.size());
        for(const auto & requested_value : requested_values)
            requested_subscriptions.insert(SubscriptionKeyFor(requested_value));

        std::lock_guard<std::mutex> lock(ui_client_mutex);
        auto & client_state = ui_client_states[client_id];
        const bool subscriptions_changed = client_state.keys != requested_subscriptions;
        if(subscriptions_changed)
            ++ui_subscription_revision;
        client_state.keys = std::move(requested_subscriptions);
        client_state.last_seen_time = steady_clock::now();
        return subscriptions_changed;
    }


    std::shared_ptr<const Kernel::UISnapshot>
    Kernel::CurrentUISnapshot()
    {
        std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
        return current_ui_snapshot;
    }


    std::string
    Kernel::BuildUIDataResponse(const std::string & status,
                                const std::vector<DataSnapshotItem> & response_items,
                                const std::string & log_json) const
    {
        std::string response = "{\n";
        response += status;
        response += "\t\"data\":\n\t{\n";

        std::string sep;
        for(const auto & item : response_items)
        {
            if(item.value.empty())
                continue;
            response += sep;
            response += item.prefix;
            response += item.value;
            sep = ",\n";
        }

        response += "\n\t}";
        response += log_json.empty() ? ",\n\"log\": []" : log_json;
        response += ",\n\t\"has_data\": 1\n";
        response += "}\n";
        return response;
    }


    void
    Kernel::DoSendData(Request & request, bool refresh_paused_snapshot, bool use_snapshot_status)
    {
        auto requested_values = ParseRequestedUIValues(request);
        const bool client_subscriptions_changed =
            UpdateUIClientSubscriptions(request.client_id, requested_values);

        if((refresh_paused_snapshot && run_mode.load() == run_mode_pause) ||
           (use_snapshot_status && client_subscriptions_changed))
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            BuildUISnapshot();
        }

        std::shared_ptr<const UISnapshot> snapshot = CurrentUISnapshot();

        long response_session_id = 0;
        std::string status;
        std::string log_json = ConsumeLogForClient(request.client_id);
        if(snapshot != nullptr)
        {
            response_session_id = snapshot->session_id;
            if(use_snapshot_status)
                status = snapshot->status_json;
        }

        std::vector<DataSnapshotItem> response_items;
        std::vector<std::pair<size_t, RequestedUIValue>> fallback_items;
        response_items.reserve(requested_values.size());

        for(const auto & requested_value : requested_values)
        {
            response_items.push_back({
                "\t\t\"" + escape_json_string(requested_value.key) + "\": ",
                ""
            });

            if(snapshot != nullptr)
            {
                auto it = snapshot->serialized_values.find(SubscriptionKeyFor(requested_value));
                if(it != snapshot->serialized_values.end())
                {
                    response_items.back().value = it->second;
                    continue;
                }
            }

            fallback_items.emplace_back(response_items.size() - 1, requested_value);
        }

        bool serialize_live_status = snapshot == nullptr || !use_snapshot_status;
        if(!fallback_items.empty() || serialize_live_status)
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            if(snapshot == nullptr)
                response_session_id = session_id;
            if(serialize_live_status)
                status = DoSendDataStatus();

            for(const auto & fallback_item : fallback_items)
            {
                if(run_mode == run_mode_realtime && tick_duration > 0 && intra_tick_timer.GetTime() >= tick_duration)
                {
                    Notify(msg_debug, "Stopped sending data before next realtime tick.");
                    break;
                }

                try
                {
                    std::string serialized_value;
                    if(SerializeRequestedValue(fallback_item.second, serialized_value))
                        response_items[fallback_item.first].value = std::move(serialized_value);
                }
                catch(const std::exception & e)
                {
                    Notify(msg_warning, "Could not send data for \"" + fallback_item.second.key + "\": " + std::string(e.what()));
                }
            }
        }

        dictionary header({
            {"Session-Id", std::to_string(response_session_id)},
            {"Package-Type", "data"},
            {"Content-Type", "application/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"},
            {"Expires", "0"}
        });

        SendStringResponse(header, BuildUIDataResponse(status, response_items, log_json));
    }


    std::string
    Kernel::SendImage(const matrix & image, const std::string & format, int quality) // Compress image to jpg and return a base64 data URI
    {
        jpeg_data jpeg;

        if(format=="rgb" && image.rank() == 3 && image.size(0) == 3)
            jpeg = create_color_jpeg(image, quality);

        else if(format=="gray" && image.rank() == 2)
            jpeg = create_gray_jpeg(image, 0, 1, quality);

        else if(image.rank() == 2) // taking our chances with the format...
            jpeg = create_pseudocolor_jpeg(image, 0, 1, format, quality);

        if(jpeg.empty())
            return "\"\"";

        const std::string jpeg_base64 = base64_encode(jpeg.data(), jpeg.size());
        std::string result = "\"data:image/jpeg;base64,";
        result += jpeg_base64;
        result += "\"";
        return result;
    }


}; // namespace ikaros
