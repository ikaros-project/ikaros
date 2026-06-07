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
    preserve_clean_new: false,
    clean_new_session_id: null,
    save_in_flight: false,
    save_pending: false,
    reset_layout_on_network: false,
    slow_ui_delay_ms: 0,
    slow_ui_jitter_ms: 0,
    drop_update_rate: 0,
    data_package: {},

    cloneNetworkForStorage(source)
    {
        const clone = deepCopy(source);
        if(clone && typeof clone === "object")
            delete clone.log;
        return clone;
    },

    normalizeSaveFilename(filename)
    {
        if(filename == null)
            return "";

        let text = String(filename).trim();
        if(text === "" || text === "null" || text === "undefined")
            return "";

        text = text.replace(/\\/g, "/");
        const parts = text.split("/").filter(part => part.length > 0);
        text = parts.length > 0 ? parts[parts.length - 1] : text;
        text = text.trim();

        if(text === "" || text === "." || text === "..")
            return "";

        if(text.toLowerCase().endsWith(".ikg"))
            text = text.slice(0, -4);

        return text.trim();
    },

    reconnect()
    {
        if(window.auth && typeof auth.resumeAfterReconnect === "function")
            auth.resumeAfterReconnect();
        else
        {
            controller.get("update", controller.update);
            controller.setSystemInfo(null);
            controller.setWaitingState();
        }
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
        if(network.tainted)
            controller.clean_new_session_id = null;
        controller.syncTransportButtons();
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
        if(controller.save_in_flight)
        {
            controller.save_pending = true;
            return;
        }

        controller.save_in_flight = true;
        let jsonString = "";
        try
        {
            if(main && typeof main.savePaneLayout === "function")
                main.savePaneLayout();
            const networkCopy = controller.cloneNetworkForStorage(network.network);
            const filename = controller.normalizeSaveFilename(networkCopy ? networkCopy.filename : "");
            if(filename === "")
                throw new Error("No valid filename is set.");
            networkCopy.filename = filename;
            if(network && network.network)
                network.network.filename = filename;
            jsonString = JSON.stringify(networkCopy,null,2);
        }
        catch(error)
        {
            controller.save_in_flight = false;
            const message = error && error.message ? error.message : String(error);
            console.error("Save serialization failed:", error);
            alert("Save failed: " + message);
            return;
        }

        const finish = function()
        {
            controller.save_in_flight = false;
            if(controller.save_pending)
            {
                controller.save_pending = false;
                controller.save();
            }
        };

        const saveSucceeded = function(data)
        {
            if(data && data.filename && network && network.network)
                network.network.filename = data.filename;
            main.setViewMode();
            controller.setTainted(false, "saveNetwork success");
            controller.data_package = data;
            controller.get("network", controller.update);
            finish();
        };

        const saveFailed = function(error)
        {
            console.error('Error:', error);
            const message = error && error.message ? error.message : String(error);
            alert("Save failed: " + message);
            finish();
        };

        const maxSaveAttempts = 5;
        const saveRetryDelays = [150, 350, 800, 1600];
        const sendSaveRequest = function(attempt)
        {
            const xhr = new XMLHttpRequest();
            xhr.open("PUT", "/save?request=" + Date.now() + "-" + attempt, true);
            xhr.setRequestHeader("Content-Type", "application/json");
            xhr.setRequestHeader("Session-Id", controller.session_id);
            xhr.setRequestHeader("Client-Id", controller.client_id);
            xhr.timeout = 10000;

            xhr.onload = function()
            {
                if(xhr.status === 0)
                {
                    retryOrFail(new Error("Network error while saving."));
                    return;
                }
                if(xhr.status < 200 || xhr.status >= 300)
                {
                    saveFailed(new Error(`Save failed with HTTP ${xhr.status} ${xhr.statusText}: ${xhr.responseText}`));
                    return;
                }

                try
                {
                    saveSucceeded(JSON.parse(xhr.responseText));
                }
                catch(error)
                {
                    saveFailed(new Error(`Save response was not JSON: ${xhr.responseText}`));
                }
            };

            const retryOrFail = function(error)
            {
                if(attempt + 1 < maxSaveAttempts)
                {
                    const delay = saveRetryDelays[Math.min(attempt, saveRetryDelays.length - 1)];
                    setTimeout(function() { sendSaveRequest(attempt + 1); }, delay);
                    return;
                }
                saveFailed(error);
            };

            xhr.onerror = function()
            {
                retryOrFail(new Error("Network error while saving."));
            };

            xhr.ontimeout = function()
            {
                retryOrFail(new Error("Save request timed out."));
            };

            try
            {
                xhr.send(jsonString);
            }
            catch(error)
            {
                retryOrFail(error);
            }
        };

        sendSaveRequest(0);
    },

    commitActiveEditBeforeSave()
    {
        if(main && main.inline_name_edit)
            main.finishInlineNameEdit(true);

        const activeElement = document.activeElement;
        if(activeElement && activeElement !== document.body && typeof activeElement.blur === "function")
            activeElement.blur();
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
              if(xhr.status === 401)
              {
                  if(is_update_request)
                      controller.update_in_flight = false;
                  controller.open_mode = false;
                  auth.handleUnauthorized();
                  return;
              }

              if(is_update_request)
                  controller.update_in_flight = false;
              const artificialDelay = controller.getSlowLinkDelay();
              const finishResponse = function()
              {
                  finalizeResponse();
                  if(is_update_request && controller.commandQueue.length > 0)
                      controller.flushCommandQueue();
              };
              if(artificialDelay > 0)
                  setTimeout(finishResponse, artificialDelay);
              else
                  finishResponse();
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
    
    queueCommand(command, path="", dictionary={}, flush=false) {
        controller.commandQueue.push([command, path, dictionary]);
        if(flush)
            controller.flushCommandQueue();
    },

    flushCommandQueue()
    {
        clearTimeout(controller.request_timer);
        controller.request_timer = null;
        if(!controller.open_mode && !controller.update_in_flight)
            controller.requestUpdate();
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

    getUpdateGroupPath()
    {
        const paneCount = typeof document !== "undefined" ? document.querySelectorAll(".main_pane").length : 0;
        if(paneCount > 1 && network && network.network && network.network.name)
            return network.network.name;
        return selector.selected_background || "";
    },

    getUpdateProxyPath()
    {
        const paneCount = typeof document !== "undefined" ? document.querySelectorAll(".main_pane").length : 0;
        if(paneCount > 1)
            return "";
        return controller.getSelectedGroupProxyPath();
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

    forceNextUpdate()
    {
        controller.commandQueue = controller.commandQueue.filter((command) => command[0] !== "update");
        controller.queueCommand('update');
        clearTimeout(controller.request_timer);
        controller.request_timer = null;
        if(!controller.update_in_flight)
            controller.requestUpdate();
    },

    new() 
    {
        controller.update_generation++;
        controller.update_in_flight = false;
        controller.pending_network_session_id = null;
        controller.clean_new_session_id = null;
        controller.preserve_clean_new = true;
        controller.reset_layout_on_network = true;
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
        controller.clean_new_session_id = null;
        controller.open_mode = true;
        controller.preserve_clean_open = true;
        controller.reset_layout_on_network = true;
        dialog.showOpenDialog(controller.openCallback, "Select file to open");
    },

    save() 
    {
        controller.commitActiveEditBeforeSave();
        const filename = controller.normalizeSaveFilename(network && network.network ? network.network.filename : null);
        if(filename === "")
            controller.saveas();
        else
        {
            network.network.filename = filename;
            controller.saveNetwork();
        }
    },

    saveas() 
    {
        controller.commitActiveEditBeforeSave();
        dialog.showSaveDialog(function(filename) {
            const normalized = controller.normalizeSaveFilename(filename);
            if(normalized === "")
                return;
            network.network.filename = normalized;
            controller.save();
        }, null);
    },

    sendStateFileCommand(command, filename)
    {
        const query = filename ? "?filename=" + encodeURIComponent(filename) : "";
        fetch("/" + command + query, {
            method: "GET",
            headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}
        })
        .then(function(response) {
            if(response.status === 401)
            {
                auth.handleUnauthorized();
                return null;
            }
            if(!response.ok)
                return response.text().then(function(text) {
                    throw new Error(text || response.statusText);
                });
            return response.json();
        })
        .then(function(result) {
            if(result)
                controller.requestUpdate();
        })
        .catch(function(error) {
            window.alert(error.message || String(error));
        });
    },

    saveState(filename)
    {
        controller.sendStateFileCommand("savestate", filename);
    },

    loadState(filename)
    {
        if(!window.confirm("Load state for this network?"))
            return;
        controller.sendStateFileCommand("loadstate", filename);
    },

    quit() {
        controller.run_mode = 'quit';
        controller.sendRunModeCommand("quit");
     },

    stop() {
        controller.run_mode = 'stop';
        controller.syncTransportButtons();
        controller.sendRunModeCommand("stop");
     },

    sendRunModeCommand(command)
    {
        if(controller.isRunModeBlockedByTaintedNetwork(command))
        {
            controller.syncTransportButtons();
            return;
        }

        controller.update_generation++;
        clearTimeout(controller.request_timer);
        controller.request_timer = null;
        const requestedData = controller.getRequestedDataString();
        const params = {};
        if(requestedData !== "")
            params.data = requestedData;
        const url_params = toURLParams(params);
        controller.get(command + (url_params !== "" ? "?" + url_params : ""), controller.update);
    },
    
    pause()
    {
        if(controller.isRunModeBlockedByTaintedNetwork("pause"))
            return;

        controller.run_mode = 'pause';
        controller.syncTransportButtons();
        document.querySelector("#state").innerText = "pause";
        main.setViewMode();
        controller.sendRunModeCommand("pause");
    },
    
    step()
    {
        if(controller.isRunModeBlockedByTaintedNetwork("step"))
            return;

        controller.run_mode = 'step';
        controller.syncTransportButtons();
        document.querySelector("#state").innerText = "step";
        main.setViewMode();
        controller.sendRunModeCommand("step");
    },
    
    play()
    {
        if(controller.isRunModeBlockedByTaintedNetwork("play"))
            return;
    
        controller.run_mode = 'play';
        controller.syncTransportButtons();
        main.setViewMode()
        controller.sendRunModeCommand("play");
    },
    
    realtime()
    {
        if(controller.isRunModeBlockedByTaintedNetwork("realtime"))
            return;
            
        controller.run_mode = 'realtime';
        controller.syncTransportButtons();
        main.setViewMode();
        controller.sendRunModeCommand("realtime");
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
                const widgetElement = main.getFrameWidget(w[i]);
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
                document.querySelector("#top_tick").innerText = "-";
                document.querySelector("#state").innerText = "-";
                document.querySelector("#uptime").innerText = "-";
                document.querySelector("#time").innerText = "-";
                document.querySelector("#top_time").innerText = "-";
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
            const responseRunMode = ['quit', 'stop','pause','play','realtime','restart'][response.state];
            if(responseRunMode)
                controller.run_mode = responseRunMode;
            document.querySelector("#file").innerText = displayedFile;
            const displayedTick = (Number.isInteger(response.tick) && response.tick >= 0 ?  response.tick : "-");
            document.querySelector("#tick").innerText = displayedTick;
            document.querySelector("#top_tick").innerText = displayedTick;
            document.querySelector("#state").innerHTML = controller.run_mode + (network.tainted ? " <span style='color:red'>&#9679;</span>" : "");

            document.querySelector("#uptime").innerText = secondsToHMS(response.uptime);
            const displayedTime = secondsToHMS(response.time);
            document.querySelector("#time").innerText = displayedTime;
            document.querySelector("#top_time").innerText = displayedTime;
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
            controller.syncTransportButtons();
        }
        catch(err)
        {
            console.log("controller.setSystemInfo: incorrect package received from ikaros.")
        }
    },

    syncTransportButtons()
    {
        const activeMode = controller.run_mode === "restart" ? "play" : controller.run_mode;
        document.querySelectorAll(".transport_button").forEach((button) =>
        {
            const mode = button.dataset ? button.dataset.transportMode : "";
            const blocked = controller.isRunModeBlockedByTaintedNetwork(mode);
            if(button.dataset && button.dataset.defaultTitle === undefined)
                button.dataset.defaultTitle = button.getAttribute("title") || "";
            button.classList.toggle("transport-blocked", blocked);
            if(blocked)
            {
                button.disabled = false;
                button.removeAttribute("title");
                button.setAttribute("aria-disabled", "true");
                button.setAttribute("aria-label", "Save before continuing");
                button.dataset.disabledReason = "Save before continuing";
            }
            else
            {
                button.disabled = false;
                button.removeAttribute("aria-disabled");
                button.setAttribute("title", button.dataset ? button.dataset.defaultTitle : "");
                button.removeAttribute("data-disabled-reason");
            }
            button.classList.toggle("transport-active", mode === activeMode);
        });
        document.querySelectorAll(".top_transport_controls").forEach((controls) =>
        {
            const blocked = !!(network && network.tainted);
            controls.classList.toggle("transport-controls-blocked", blocked);
            if(!controls.dataset.saveBlockedHandlerInstalled)
            {
                controls.addEventListener("click", function(evt)
                {
                    if(!controls.classList.contains("transport-controls-blocked"))
                        return;
                    evt.preventDefault();
                    evt.stopPropagation();
                    controller.save();
                }, true);
                controls.dataset.saveBlockedHandlerInstalled = "true";
            }
        });
    },

    isRunModeBlockedByTaintedNetwork(command)
    {
        return !!(network && network.tainted && ["pause", "step", "play", "realtime"].includes(command));
    },

    update(response, session_id, package_type)
    {
        if(isEmpty(response))
        {
            console.log("ERROR: empty or malformed response");
            return;
        }

        const isNetworkPackage = package_type == "network" || response._tag == "group";
        const sessionChanged = (!isNetworkPackage && controller.session_id != session_id);

        if(main && typeof main.hideReconnectOverlay === "function")
            main.hideReconnectOverlay();
        controller.ping = Date.now() - controller.send_stamp;
        controller.defer_reconnect();
        if(controller.request_timer === null && !controller.open_mode)
            controller.requestUpdate();

        if(response.log)
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

        if(isNetworkPackage)
        {
            const wasOpenRequest = controller.open_mode;
            const wasNewRequest = controller.preserve_clean_new;
            controller.open_mode = false;
            controller.preserve_clean_new = false;
            controller.update_in_flight = false;
            controller.pending_network_session_id = null;
            controller.clearQueue();
            controller.session_id = session_id;
            document.querySelector("header").style.display="block";
            document.querySelector("#load").style.display="none";
   
            controller.tick = response.tick;
            network.init(controller.cloneNetworkForStorage(response));
            if(main && typeof main.applyStartupTopChromeVisibility === "function")
                main.applyStartupTopChromeVisibility();
            const shouldResetLayout = wasOpenRequest || controller.reset_layout_on_network;
            controller.reset_layout_on_network = false;
            if(wasNewRequest && main && typeof main.clearSavedPaneLayout === "function")
                main.clearSavedPaneLayout();
            if((wasOpenRequest || wasNewRequest) && main.main)
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
            let restoredPaneLayout = false;
            if(!wasNewRequest && main && typeof main.restorePaneLayout === "function")
                restoredPaneLayout = main.restorePaneLayout();
            if(shouldResetLayout && !restoredPaneLayout && main && typeof main.resetSplitLayout === "function")
                main.resetSplitLayout();

            let v = restoredPaneLayout && main && main.active_pane && main.active_pane.root ? main.getPaneBackground(main.active_pane.root) : getCookie('selected_background');
            if(v && network.dict[v])
                selector.selectItems([], v, false, false, true);
            else
                selector.selectItems([], top, false, false, true);

            if(shouldAutoArrange)
                main.arrangeComponents();
            if(wasNewRequest)
                controller.clean_new_session_id = session_id;
            const isCleanNewSession = controller.clean_new_session_id === session_id;
            if(wasOpenRequest || isCleanNewSession || controller.preserve_clean_open)
                controller.setTainted(false, isCleanNewSession ? "network package from explicit new" : (wasOpenRequest ? "network package from explicit open" : "network package during preserved explicit open"));
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
            if((wasOpenRequest || isCleanNewSession || controller.preserve_clean_open) && !network.tainted)
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

        group_path = controller.getUpdateGroupPath();
        const group_proxy_path = controller.getUpdateProxyPath();
        const data_string = controller.getRequestedDataString();

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

    getRequestedDataString()
    {
        let data_set = new Set();
        const w = document.getElementsByClassName('frame');
        for(let i=0; i<w.length; i++)
            try
            {
                const widgetElement = main.getFrameWidget(w[i]);
                if(widgetElement && 'requestData' in widgetElement)
                    widgetElement.requestData(data_set);
            }
            catch(err)
            {
                console.log("requestData failed: "+err);
            }

        let data_string = "";
        let sep = "";
        for(const s of data_set)
        {
            if(s !== "")
            {
                data_string += (sep + s);
                sep = ",";
            }
        }
        return data_string;
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
                const widgetElement = main.getFrameWidget(w[i]);
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
