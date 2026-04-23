const controller = 
{
    run_mode: 'pause',
    commandQueue: [['update', "",{}]],
    tick: 0,
    session_id: 0,
    client_id: Date.now(),
    files: {},
    load_count: 0,
    load_count_timeout: null,
    g_data: null,
    send_stamp: 0,
    tick_duration: 0,
    webui_interval: 0,
    webui_req_int: 100,
    timeout: 500,
    reconnect_interval: 1200,
    reconnect_timer: null,
    request_timer: null,
    update_generation: 0,
    update_in_flight: false,
    pending_network_session_id: null,
    open_mode: false,
    preserve_clean_open: false,
    slow_ui_delay_ms: 0,
    slow_ui_jitter_ms: 0,
    drop_update_rate: 0,
    data_package: {},

    reconnect()
    {
        controller.get("update", controller.update);
        controller.setSystemInfo(null);
        controller.setWaitingState();
    },

    defer_reconnect()
    {
        clearInterval(controller.reconnect_timer);
        controller.reconnect_timer = null;
    },

    schedule_reconnect()
    {
        clearTimeout(controller.request_timer);
        controller.request_timer = null;
        controller.update_in_flight = false;
        if(controller.reconnect_timer !== null)
            return;
        controller.setSystemInfo(null);
        controller.setWaitingState();
        controller.reconnect_timer = setInterval(controller.reconnect, controller.reconnect_interval);
    },

    setWaitingState()
    {
        const stateElement = document.querySelector("#state");
        if(stateElement)
            stateElement.innerHTML = 'waiting to reconnect <span class="status-spinner" aria-hidden="true"></span>';
        if(main && typeof main.showReconnectOverlay === "function")
            main.showReconnectOverlay();
    },

    setTainted(nextValue, reason = "")
    {
        network.tainted = !!nextValue;
    },

    // Dev-only URL params for resilience testing, e.g.
    // ?slow_ui=200&slow_ui_jitter=100&drop_updates=0.1
    getURLNumberParam(name, defaultValue=0)
    {
        const value = new URLSearchParams(window.location.search).get(name);
        if(value === null || value === "")
            return defaultValue;
        const parsed = Number(value);
        return Number.isFinite(parsed) ? parsed : defaultValue;
    },

    configureSlowLinkMode()
    {
        controller.slow_ui_delay_ms = Math.max(0, controller.getURLNumberParam("slow_ui", 0));
        controller.slow_ui_jitter_ms = Math.max(0, controller.getURLNumberParam("slow_ui_jitter", 0));
        controller.drop_update_rate = Math.min(1, Math.max(0, controller.getURLNumberParam("drop_updates", 0)));

        if(controller.slow_ui_delay_ms > 0 || controller.slow_ui_jitter_ms > 0 || controller.drop_update_rate > 0)
        {
            const parts = [];
            if(controller.slow_ui_delay_ms > 0)
                parts.push(`delay=${controller.slow_ui_delay_ms}ms`);
            if(controller.slow_ui_jitter_ms > 0)
                parts.push(`jitter=${controller.slow_ui_jitter_ms}ms`);
            if(controller.drop_update_rate > 0)
                parts.push(`drop_updates=${controller.drop_update_rate}`);
            log.print(`Slow-link test mode active (${parts.join(", ")}).`);
        }
    },

    getSlowLinkDelay()
    {
        if(controller.slow_ui_delay_ms <= 0 && controller.slow_ui_jitter_ms <= 0)
            return 0;

        const jitter = controller.slow_ui_jitter_ms > 0 ? Math.random() * controller.slow_ui_jitter_ms : 0;
        return Math.round(controller.slow_ui_delay_ms + jitter);
    },

    saveNetwork()
    {
        const jsonString = JSON.stringify(network.network,null,2);

        fetch("save", {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json',
                'Session-Id': controller.session_id,
                'Client-Id': controller.client_id
            },
            body: jsonString
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok ' + response.statusText);
            }
            return response.json();
        })
        .then(data => {
            main.setViewMode();
            controller.setTainted(false, "saveNetwork success");
            controller.data_package = data;
            controller.setSystemInfo(data);
            controller.get("update", controller.update);
        })
        .catch(error => {
            console.error('Error:', error);
            alert("Save failed");
        });
    },

    get (url, callback)
    {
        controller.send_stamp = Date.now();
        const request_update_generation = controller.update_generation;
        const request_session_id = controller.session_id;
        const is_update_request = url.startsWith("update");
        let xhr = new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.setRequestHeader("Session-Id", controller.session_id);
        xhr.setRequestHeader("Client-Id", controller.client_id);
        if(is_update_request)
            controller.update_in_flight = true;
        const finalizeResponse = function()
        {
            if(
                is_update_request &&
                (
                    request_update_generation !== controller.update_generation ||
                    request_session_id !== controller.session_id
                )
            )
                return;

            if(is_update_request && controller.drop_update_rate > 0 && Math.random() < controller.drop_update_rate)
                return;

            callback(xhr.response, xhr.getResponseHeader("Session-Id"), xhr.getResponseHeader("Package-Type"));
        };
        xhr.onload = function(evt)
        {
              if(is_update_request)
                  controller.update_in_flight = false;
              const artificialDelay = controller.getSlowLinkDelay();
              if(artificialDelay > 0)
                  setTimeout(finalizeResponse, artificialDelay);
              else
                  finalizeResponse();
        }
        
        xhr.ontimeout = function() 
        {
            if(is_update_request)
                controller.update_in_flight = false;
            controller.open_mode = false;
            controller.schedule_reconnect();
        }

        xhr.onerror = function()
        {
            if(is_update_request)
                controller.update_in_flight = false;
            controller.open_mode = false;
            controller.schedule_reconnect();
        };

        xhr.responseType = 'json';
        xhr.timeout = 2000;
        try {
            xhr.send();
        }
        catch(error)
        {
            if(is_update_request)
                controller.update_in_flight = false;
            console.log("console.get: "+error);
        }
    },

    init()
    {
        controller.configureSlowLinkMode();
        controller.getClasses();
        controller.getClassInfo();
        controller.requestUpdate();
    },
    
    queueCommand(command, path="", dictionary={}) {
        controller.commandQueue.push([command, path, dictionary]);
    },

    getSelectedGroupProxyPath()
    {
        const groupPath = selector.selected_background || "";
        if(groupPath === "" || !network || !network.dict || !network.dict[groupPath])
            return "";

        const proxyPath = network.dict[groupPath].proxy;
        if(typeof proxyPath !== "string")
            return "";

        const trimmedProxyPath = proxyPath.trim();
        if(trimmedProxyPath === "")
            return "";

        return trimmedProxyPath[0] === "." ? trimmedProxyPath.substring(1) : trimmedProxyPath;
    },

    clearQueue()
    {
        controller.commandQueue = [];
    },

    ensureUpdateQueued()
    {
        for(const command of controller.commandQueue)
            if(command[0] === "update")
                return;
        controller.queueCommand('update');
    },

    new() 
    {
        controller.update_generation++;
        controller.update_in_flight = false;
        controller.pending_network_session_id = null;
        controller.clearQueue();
        controller.get("new", controller.update);
    },

    openCallback(filename, where)
    {
        controller.get("open?where="+where+"&file="+filename, controller.update);
    },

    open() 
    {
        controller.update_generation++;
        controller.update_in_flight = false;
        controller.pending_network_session_id = null;
        controller.open_mode = true;
        controller.preserve_clean_open = true;
        dialog.showOpenDialog(controller.openCallback, "Select file to open");
    },

    save() 
    {
        const filename = network.network.filename;
        if(filename == null || filename === "" || filename === "null")
            controller.saveas();
        else
            controller.saveNetwork();
    },

    saveas() 
    {
        dialog.showSaveDialog(function(filename) {
            network.network.filename = filename;
            controller.save();
        }, null);
    },

    quit() {
        controller.run_mode = 'quit';
        controller.get("quit", controller.update);
     },

    stop() {
        controller.run_mode = 'stop';
        controller.get("stop", controller.update);
     },
    
    pause()
    {
        if(network.tainted)
            return;

        main.setViewMode();
        controller.queueCommand('pause');
    },
    
    step()
    {
        if(network.tainted)
            return;
        
        document.querySelector("#state").innerText = "step";
        main.setViewMode();
        controller.queueCommand('step');
    },
    
    play()
    {
        if(network.tainted)
            return;
    
        main.setViewMode()
        controller.queueCommand('play');
    },
    
    realtime()
    {
        if(network.tainted)
            return;
            
        main.setViewMode();
        controller.queueCommand('realtime');
    },

    clear_wait()
    {
        controller.load_count = 0;
    },

    wait_for_load(data)
    {
        if(controller.load_count > 0)
            setTimeout(controller.wait_for_load, 1);
        else
        {
            clearTimeout(controller.load_count_timeout);
            controller.updateWidgets(controller.g_data);
        }
    },
    
    updateImages(data)
    {
        controller.load_count = 0;
        controller.g_data = data;

        try
        {
            let w = document.getElementsByClassName('frame')
            for(let i=0; i<w.length; i++)
            {
                const widgetElement = w[i].widget;
                if(widgetElement && widgetElement.loadData)
                    controller.load_count += widgetElement.loadData(data);
            }
     
            controller.load_count_timeout = setTimeout(controller.clear_wait, 200);
            setTimeout(controller.wait_for_load, 1);
        }
        catch(err)
        {
            console.log("updateImage: exception: "+err);
        }
    },

    setSystemInfo(response)
    {
        try
        {
            if(response === null)
            {
                document.querySelector("#file").innerText = "-";
                document.querySelector("#tick").innerText = "-";
                document.querySelector("#state").innerText = "-";
                document.querySelector("#uptime").innerText = "-";
                document.querySelector("#time").innerText = "-";
                document.querySelector("#ticks_per_s").innerText = "-";
                document.querySelector("#tick_duration").innerText = "-";
                document.querySelector("#actual_duration").innerText =  "-";
                document.querySelector("#lag").innerText = "-";
                document.querySelector("#cpu_cores").innerText = "-";
                document.querySelector("#time_usage").innerText = "-";
                document.querySelector("#usage").innerText = "-";
                document.querySelector("#webui_updates_per_s").innerText = "-";
                document.querySelector("#webui_interval").innerText = "-";
                document.querySelector("#webui_req_int").innerText = "-";
                document.querySelector("#webui_ping").innerText = "-";
                document.querySelector("#webui_lag").innerText = "-";
                let p = document.querySelector("#progress");
                p.style.display = "none";
                return;
            }

            if(response.debug)
                document.querySelector("#debug_row").style.display="table-row";
            else
                document.querySelector("#debug_row").style.display="none";

            const displayedFile =
                (network && network.network && network.network.filename != null && network.network.filename !== "" && network.network.filename !== "null") ? network.network.filename :
                (response.filename != null && response.filename !== "" && response.filename !== "null") ? response.filename :
                "-";
            document.querySelector("#file").innerText = displayedFile;
            document.querySelector("#tick").innerText = (Number.isInteger(response.tick) && response.tick >= 0 ?  response.tick : "-");
            document.querySelector("#state").innerHTML = controller.run_mode + (network.tainted ? " <span style='color:red'>&#9679;</span>" : "");

            document.querySelector("#uptime").innerText = secondsToHMS(response.uptime);
            document.querySelector("#time").innerText = secondsToHMS(response.time);
            document.querySelector("#ticks_per_s").innerText = response.ticks_per_s;
            document.querySelector("#tick_duration").innerHTML = formatTime(response.tick_duration);
            document.querySelector("#actual_duration").innerHTML = formatTime(response.actual_duration);
            document.querySelector("#lag").innerHTML = formatTime(response.lag);
            document.querySelector("#cpu_cores").innerText = response.cpu_cores;
            document.querySelector("#time_usage").value = response.time_usage;
            document.querySelector("#usage").value = response.cpu_usage;
            if(Number.isFinite(response.webui_req_int))
                controller.webui_req_int = Math.max(1, Math.round(response.webui_req_int * 1000));
            document.querySelector("#webui_updates_per_s").innerText = (1000/controller.webui_interval).toFixed(1) + (response.has_data ? "": " (no data)");
            document.querySelector("#webui_interval").innerText = controller.webui_interval+" ms";
            document.querySelector("#webui_req_int").innerText = (controller.webui_req_int / 1000)+" s";
            document.querySelector("#webui_ping").innerText = controller.ping+" ms";
            document.querySelector("#webui_lag").innerText = (Date.now()-response.timestamp)+" ms";
            
            let p = document.querySelector("#progress");
            if(response.progress > 0)
            {
                p.value = response.progress;
                p.style.display = "table-row";
            }
            else
                p.style.display = "none";

            controller.tick = response.tick;
            controller.run_mode = ['quit', 'stop','pause','play','realtime','restart'][response.state];

            controller.syncTransportButtons();
        }
        catch(err)
        {
            console.log("controller.setSystemInfo: incorrect package received from ikaros.")
        }
    },

    syncTransportButtons()
    {
        document.querySelectorAll(".transport_button").forEach((button) =>
        {
            button.disabled = !!network.tainted;
        });
    },

    update(response, session_id, package_type)
    {
        if(isEmpty(response))
        {
            console.log("ERROR: empty or malformed response");
            return;
        }

        const sessionChanged = (package_type !== "network" && controller.session_id != session_id);

        if(main && typeof main.hideReconnectOverlay === "function")
            main.hideReconnectOverlay();
        controller.ping = Date.now() - controller.send_stamp;
        controller.defer_reconnect();
        if(controller.request_timer === null && !controller.open_mode)
            controller.requestUpdate();

        if(response.log && !sessionChanged)
        {
            let logElement = log.getMessagesElement();
            if(logElement)
            {
                const fragment = document.createDocumentFragment();
                let shouldScrollLog = false;
                response.log.forEach((element) => 
                {
                    let message_class = ["inherit", "quiet","exception","end_of_file","terminate","fatal_error","warning","print","debug","trace"][element[0]];
                    let p = document.createElement('p');
                    p.className = message_class;
                    p.appendChild(document.createTextNode(element[1]));
                    if(element[2])
                    {
                        let a = document.createElement('a');
                        a.href = '#';
                        a.innerText = " \u00bb " + element[2];
                        a.onclick = function (evt)
                        {
                            evt.preventDefault();
                            selector.selectError(element[2]);
                        };
                        p.appendChild(a);
                    }
                    fragment.appendChild(p);
                    if(element[0]<=6)
                    {
                        log.showView(true);
                        shouldScrollLog = true;
                    }
                });
                logElement.appendChild(fragment);
                if(shouldScrollLog)
                    logElement.scrollTop = logElement.scrollHeight;
            }
        }

        if(package_type == "network")
        {
            const wasOpenRequest = controller.open_mode;
            controller.open_mode = false;
            controller.update_in_flight = false;
            controller.pending_network_session_id = null;
            controller.clearQueue();
            controller.session_id = session_id;
            document.querySelector("header").style.display="block";
            document.querySelector("#load").style.display="none";
   
            controller.tick = response.tick;
            network.init(response);
            if(wasOpenRequest && main.main)
            {
                main.main.classList.add("view_mode");
                main.main.classList.remove("edit_mode");
                main.edit_mode = false;
                if(inspector && typeof inspector.updateLibraryAddButtonState === "function")
                    inspector.updateLibraryAddButtonState();
            }
            const shouldAutoArrange = !network.hasAnyComponentPosition();
            nav.populate();
            let top = network.network.name;

            let v = getCookie('selected_background');
            if(v && network.dict[v])
                selector.selectItems([], v, false, false, true);
            else
                selector.selectItems([], top, false, false, true);

            if(shouldAutoArrange)
                main.arrangeComponents();
            if(wasOpenRequest || controller.preserve_clean_open)
                controller.setTainted(false, wasOpenRequest ? "network package from explicit open" : "network package during preserved explicit open");
            else
            {
                const resolvedFilename =
                    (network && network.network && network.network.filename != null) ? network.network.filename :
                    response.filename;
                const hasOpenedFilename = (
                    resolvedFilename != null &&
                    String(resolvedFilename) !== "" &&
                    String(resolvedFilename) !== "null"
                );
                controller.setTainted(!hasOpenedFilename, "network package filename heuristic");
            }
            if((wasOpenRequest || controller.preserve_clean_open) && !network.tainted)
                main.setViewMode();
            controller.syncTransportButtons();
       }

        else if(sessionChanged)
        {
            if(controller.pending_network_session_id === session_id)
                return;
            controller.pending_network_session_id = session_id;
            controller.get("network", controller.update);
            return;
        }

        else if(package_type == "data")
        {
            if(controller.preserve_clean_open)
            {
                controller.setTainted(false, "open data preserve_clean_open");
                controller.syncTransportButtons();
                controller.preserve_clean_open = false;
            }
            controller.data_package = response
            controller.setSystemInfo(response);
            controller.tick_duration = response.tick_duration || 0;
            if(response.has_data)
            {

                if(!response.data || Object.keys(response.data).length === 0)
                {
                }
                else
                {
                    controller.updateImages(response.data);
                }
            }
        }
    },

    requestUpdate()
    {
        clearTimeout(controller.request_timer);
        controller.request_timer = setTimeout(controller.requestUpdate, controller.webui_req_int);

        if(controller.open_mode)
            return;

        if(controller.update_in_flight)
        {
            controller.ensureUpdateQueued();
            return;
        }

        controller.webui_interval = Date.now() - controller.last_request_time;
        controller.last_request_time = Date.now();

        let group_path ="";
        let data_string = "";

        let data_set = new Set();
        const w = document.getElementsByClassName('widget');
        for(let i=0; i<w.length; i++)
            try
            {
                const frame = w[i].closest('.frame');
                const widgetElement = frame ? frame.widget : null;
                if(widgetElement && 'requestData' in widgetElement)
                    widgetElement.requestData(data_set);
            }
            catch(err)
            {
                console.log("requestData failed: "+err);
            }

        group_path = selector.selected_background || "";
        const group_proxy_path = controller.getSelectedGroupProxyPath();
        data_string = "";
        let sep = "";
        for(const s of data_set)
        {
            if(s !== "")
            {
                data_string += (sep + s);
                sep = ","
            }
         }

         while(controller.commandQueue.length>0)
        {
            const cmd_dict = controller.commandQueue.shift();
            let s = cmd_dict[0];
            const path = cmd_dict[1];
            const dict = cmd_dict[2];
            if(data_string !== "")
                dict.data = data_string;
            if(s === "update" && group_proxy_path !== "")
                dict.proxy = group_proxy_path;
            const url_params = toURLParams(dict);

            if(path.length > 0 && path[0] === '.')
            {
                s += "/" + path;
            }
            else
            {
                if(group_path !== "")
                    s += "/." + group_path;
                else if(path !== "")
                    s += "/.";

                if(path !== "")
                    s += "." + path;
            }

            if(url_params != "")
                s += "?" + url_params;

            controller.get(s, controller.update);
        }

        controller.ensureUpdateQueued();
    },

    getClasses()
    {
        fetch('/classes', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if (!response.ok) {
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            network.classes = json.classes.sort();
            if(inspector && typeof inspector.renderLibraryClassList === "function")
                inspector.renderLibraryClassList();
        })
        .catch(function () {
            console.log("Could not get class list from server.");
        })
    },

    getClassInfo()
    {
        fetch('/classinfo', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if (!response.ok) {
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            network.classinfo = json;
        })
        .catch(function () {
            console.log("Could not get class list from server.");
        })
    },

    getFiles()
    { 
        fetch('/files', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if (!response.ok) {
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            controller.filelist = json;
        })
        .catch(function () {
            console.log("Could not get file list from server.");
        })
    },

    updateWidgets(data)
    {
        let w = document.getElementsByClassName('frame')

        for(let i=0; i<w.length; i++)
            try
            {
                const widgetElement = w[i].widget;
                if(!widgetElement)
                    continue;
                widgetElement.receivedData = data;
                widgetElement.update(data);
            }
            catch(err)
            {
                console.log("updateWidgets failed: "+controller.client_id);
            }
    },

    debug_data()
    {
        let w = window.open("about:blank", "", "_blank");
        w.document.write('<html><body><pre>'+JSON.stringify(controller.data_package,null,2)+ '</pre></body></html>');
    },
}
