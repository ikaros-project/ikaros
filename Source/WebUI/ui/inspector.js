const inspector = 
{
    subview: {},
    current_t_body: null,
    component: null,
    library: null,
    system: null,
    selected_library_class: null,
    library_readme_cache: {},
    pending_library_readme_class: null,
    component_width_cookie: 'component_inspector_width',
    library_width_cookie: 'library_inspector_width',
    min_width: 220,
    max_width_margin: 160,
    resize_active: false,
    default_width: 300,
    last_selected_signature: "",
    profiling_window: null,
    profiling_timer: null,
    profiling_sort_key: "cpu_mean",
    profiling_sort_direction: "desc",
    startup_steps_window: null,
    startup_steps_timer: null,
    startup_steps_sort_key: "path",
    startup_steps_sort_direction: "asc",

    // Initialization and top-level inspector setup.
    init()
    {
        inspector.system = document.querySelector('#system_inspector');
        inspector.component = document.querySelector('#component_inspector');
        inspector.library = document.querySelector('#library_inspector');
        inspector.libraryClassList = document.querySelector('#library_class_list');
        inspector.libraryClassDetails = document.querySelector('#library_class_details');

        inspector.subview.nothing  = document.querySelector('#inspector_nothing'); 
        inspector.subview.multiple  = document.querySelector('#inspector_multiple');
        inspector.subview.table  = document.querySelector('#inspector_table');
        inspector.subview.group_background = document.querySelector('#inspector_group_background');
        inspector.subview.group = document.querySelector('#inspector_group');
        inspector.subview.module =  document.querySelector('#inspector_module');
        inspector.subview.widget =  document.querySelector('#inspector_widget');

        inspector.hideSubviews();
        inspector.subview.nothing.style.display='table';
        inspector.updateLibraryButtonState();
        inspector.updateLibraryAddButtonState();
        inspector.renderLibraryClassList();

        inspector.setupResizeForPanel(inspector.component);
        inspector.setupResizeForPanel(inspector.library);
    },

    // Popup window template loading and profiling/module-start windows.
    loadWindowTemplate(targetWindow, templatePath, onReady)
    {
        if(!targetWindow || targetWindow.closed)
            return;

        const templateURL = new URL(templatePath, window.location.href).toString();
        const doc = targetWindow.document;
        doc.open();
        doc.write("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Loading...</title></head><body>Loading...</body></html>");
        doc.close();

        fetch(templateURL, {cache: "no-store"})
        .then((response) =>
        {
            if(!response.ok)
                throw new Error(`Window template request failed (${response.status})`);
            return response.text();
        })
        .then((html) =>
        {
            if(!targetWindow || targetWindow.closed)
                return;

            const currentDoc = targetWindow.document;
            currentDoc.open();
            currentDoc.write(html);
            currentDoc.close();
            onReady(currentDoc);
        })
        .catch((error) =>
        {
            console.log("window template load failed", error);
            if(!targetWindow || targetWindow.closed)
                return;

            const currentDoc = targetWindow.document;
            currentDoc.open();
            currentDoc.write(`<!DOCTYPE html><html><head><meta charset="utf-8"><title>Load Error</title></head><body><p>Unable to load window content.</p></body></html>`);
            currentDoc.close();
        });
    },

    getPopupWindowDocument(windowProperty)
    {
        const targetWindow = inspector[windowProperty];
        if(!targetWindow || targetWindow.closed)
            return null;
        return targetWindow.document;
    },

    initializeSortablePopupWindow(windowProperty, templatePath, buttonSelector, sortHandler, updateIndicators)
    {
        const targetWindow = inspector[windowProperty];
        if(!targetWindow || targetWindow.closed)
            return;

        inspector.loadWindowTemplate(targetWindow, templatePath, (doc) =>
        {
            doc.querySelectorAll(buttonSelector).forEach((button) =>
            {
                button.addEventListener("click", () =>
                {
                    sortHandler(button.dataset.sortKey);
                });
            });
            updateIndicators();
        });
    },

    openPollingPopupWindow(windowProperty, timerProperty, windowName, initializeWindow, refreshWindow)
    {
        if(!inspector[windowProperty] || inspector[windowProperty].closed)
        {
            inspector[windowProperty] = window.open("", windowName, "width=800,height=620,resizable=yes,scrollbars=yes");
            if(!inspector[windowProperty])
                return false;
            initializeWindow();
        }
        else
            inspector[windowProperty].focus();

        refreshWindow();

        if(inspector[timerProperty])
            clearInterval(inspector[timerProperty]);

        inspector[timerProperty] = setInterval(() =>
        {
            if(!inspector[windowProperty] || inspector[windowProperty].closed)
            {
                clearInterval(inspector[timerProperty]);
                inspector[timerProperty] = null;
                inspector[windowProperty] = null;
                return;
            }
            refreshWindow();
        }, 1000);

        return true;
    },

    openProfilingWindow()
    {
        inspector.openPollingPopupWindow(
            "profiling_window",
            "profiling_timer",
            "ikaros_profiling",
            () => inspector.initializeProfilingWindow(),
            () => inspector.refreshProfilingWindow()
        );
    },

    initializeProfilingWindow()
    {
        inspector.initializeSortablePopupWindow(
            "profiling_window",
            "profiling_window.html",
            ".profiling-sort-button",
            (sortKey) => inspector.setProfilingSort(sortKey),
            () => inspector.updateProfilingSortIndicators()
        );
    },

    profilingFormatMilliseconds(seconds)
    {
        const value = Number(seconds);
        if(!Number.isFinite(value))
            return "-";
        return (value * 1000).toFixed(3);
    },

    getProfilingSortValue(component, sortKey)
    {
        const profiling = component && component.profiling ? component.profiling : {};
        const cpu = profiling.cpu || {};
        const wall = profiling.wall || {};

        switch(sortKey)
        {
            case "path":
                return String(component && component.path ? component.path : "");
            case "class":
                return String((component && (component.class || component.name)) || "");
            case "samples":
                return Number.isFinite(cpu.count) ? cpu.count : (Number.isFinite(wall.count) ? wall.count : 0);
            case "cpu_stddev":
                return Number(cpu.standard_deviation);
            case "cpu_last":
                return Number(profiling.last_cpu_seconds);
            case "wall_mean":
                return Number(wall.mean);
            case "wall_last":
                return Number(profiling.last_wall_seconds);
            case "wall_stddev":
                return Number(wall.standard_deviation);
            case "cpu_mean":
            default:
                return Number(cpu.mean);
        }
    },

    sortProfilingComponents(components)
    {
        const direction = inspector.profiling_sort_direction === "asc" ? 1 : -1;
        const sortKey = inspector.profiling_sort_key || "cpu_mean";

        components.sort((a, b) =>
        {
            const aValue = inspector.getProfilingSortValue(a, sortKey);
            const bValue = inspector.getProfilingSortValue(b, sortKey);

            if(typeof aValue === "string" || typeof bValue === "string")
                return String(aValue).localeCompare(String(bValue)) * direction;

            const aScore = Number.isFinite(aValue) ? aValue : -Infinity;
            const bScore = Number.isFinite(bValue) ? bValue : -Infinity;
            if(aScore === bScore)
                return String(a && a.path ? a.path : "").localeCompare(String(b && b.path ? b.path : ""));
            return (aScore - bScore) * direction;
        });
    },

    setProfilingSort(sortKey)
    {
        if(inspector.profiling_sort_key === sortKey)
            inspector.profiling_sort_direction = inspector.profiling_sort_direction === "asc" ? "desc" : "asc";
        else
        {
            inspector.profiling_sort_key = sortKey;
            inspector.profiling_sort_direction = (sortKey === "path" || sortKey === "class") ? "asc" : "desc";
        }

        inspector.updateProfilingSortIndicators();
        inspector.refreshProfilingWindow();
    },

    updateProfilingSortIndicators()
    {
        const doc = inspector.getPopupWindowDocument("profiling_window");
        if(!doc)
            return;
        doc.querySelectorAll(".profiling-sort-button").forEach((button) =>
        {
            const isActive = button.dataset.sortKey === inspector.profiling_sort_key;
            button.classList.toggle("active", isActive);
            button.classList.toggle("sort-asc", isActive && inspector.profiling_sort_direction === "asc");
            button.classList.toggle("sort-desc", isActive && inspector.profiling_sort_direction === "desc");
            const baseLabel = button.dataset.label || button.textContent;
            button.dataset.label = baseLabel;
            button.textContent = baseLabel;
        });
    },

    renderProfilingWindow(data)
    {
        const doc = inspector.getPopupWindowDocument("profiling_window");
        if(!doc)
            return;
        const meta = doc.getElementById("profiling_meta");
        const rows = doc.getElementById("profiling_rows");
        if(!meta || !rows)
            return;

        const components = Array.isArray(data && data.components)
            ? data.components.filter((component) => component && component.class != null && component.class !== "")
            : [];
        inspector.sortProfilingComponents(components);
        inspector.updateProfilingSortIndicators();

        meta.textContent = `tick ${Number.isFinite(data && data.tick) ? data.tick : "-"} | updated ${new Date().toLocaleTimeString()}`;

        if(components.length === 0)
        {
            rows.innerHTML = '<tr><td colspan="7">No profiling data available.</td></tr>';
            return;
        }

        rows.innerHTML = components.map((component) =>
        {
            const profiling = component.profiling || {};
            const cpu = profiling.cpu || {};
            const wall = profiling.wall || {};
            const className = component.class || component.name || "-";
            const sampleCount = Number.isFinite(cpu.count) ? cpu.count : (Number.isFinite(wall.count) ? wall.count : 0);
            return `<tr>
<td class="path path-column" title="${inspector.escapeHTML(component.path || "-")}">${inspector.escapeHTML(component.path || "-")}</td>
<td class="class-column">${inspector.escapeHTML(className)}</td>
<td class="number">${sampleCount}</td>
<td class="number">${inspector.profilingFormatMilliseconds(cpu.mean)}</td>
<td class="number">${inspector.profilingFormatMilliseconds(cpu.standard_deviation)}</td>
<td class="number">${inspector.profilingFormatMilliseconds(wall.mean)}</td>
<td class="number">${inspector.profilingFormatMilliseconds(wall.standard_deviation)}</td>
</tr>`;
        }).join("");
    },

    showProfilingError(message)
    {
        const doc = inspector.getPopupWindowDocument("profiling_window");
        if(!doc)
            return;
        const meta = doc.getElementById("profiling_meta");
        const rows = doc.getElementById("profiling_rows");
        if(meta)
        {
            meta.textContent = message;
            meta.className = "profiling-meta profiling-error";
        }
        if(rows)
            rows.innerHTML = '<tr><td colspan="7">Unable to load profiling data.</td></tr>';
    },

    refreshProfilingWindow()
    {
        if(!inspector.getPopupWindowDocument("profiling_window"))
            return;

        fetch('/profiling', {
            method: 'GET',
            headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id},
            cache: 'no-store'
        })
        .then((response) =>
        {
            if(!response.ok)
                throw new Error(`Profiling request failed (${response.status})`);
            return response.json();
        })
        .then((data) =>
        {
            const doc = inspector.getPopupWindowDocument("profiling_window");
            const meta = doc ? doc.getElementById("profiling_meta") : null;
            if(meta)
                meta.className = "profiling-meta";
            inspector.renderProfilingWindow(data);
        })
        .catch((error) =>
        {
            console.log("profiling update failed", error);
            inspector.showProfilingError("Profiling update failed.");
        });
    },

    openStartupStepsWindow()
    {
        inspector.openPollingPopupWindow(
            "startup_steps_window",
            "startup_steps_timer",
            "ikaros_startup_steps",
            () => inspector.initializeStartupStepsWindow(),
            () => inspector.refreshStartupStepsWindow()
        );
    },

    initializeStartupStepsWindow()
    {
        inspector.initializeSortablePopupWindow(
            "startup_steps_window",
            "startup_steps_window.html",
            ".startup-steps-sort-button",
            (sortKey) => inspector.setStartupStepsSort(sortKey),
            () => inspector.updateStartupStepsSortIndicators()
        );
    },

    formatStartupStep(value)
    {
        if(value === null || value === undefined || value === "")
            return "unknown";
        const numericValue = Number(value);
        return Number.isFinite(numericValue) ? String(numericValue) : "unknown";
    },

    normalizeModuleStartValue(value)
    {
        if(value === null || value === undefined || value === "")
            return null;

        const numericValue = Number(value);
        return Number.isFinite(numericValue) ? numericValue : null;
    },

    formatModuleStart(value)
    {
        const normalizedValue = inspector.normalizeModuleStartValue(value);
        if(normalizedValue === 0)
            return "at_tick";
        if(normalizedValue === 1)
            return "first_data";
        if(normalizedValue === 2)
            return "all_data";
        return normalizedValue === null ? "unknown" : String(normalizedValue);
    },

    getStartupStepsSortValue(component, sortKey)
    {
        switch(sortKey)
        {
            case "path":
                return String(component && component.path ? component.path : "");
            case "class":
                return String((component && (component.class || component.name)) || "");
            case "module_start":
            {
                const normalizedValue = inspector.normalizeModuleStartValue(component ? component.module_start : null);
                return normalizedValue === null ? Infinity : normalizedValue;
            }
            case "start_tick":
            {
                const value = component ? component.start_tick : null;
                return value === null || value === undefined || value === "" ? Infinity : Number(value);
            }
            case "startup_all_real_inputs_step":
            {
                const value = component ? component.startup_all_real_inputs_step : null;
                return value === null || value === undefined || value === "" ? Infinity : Number(value);
            }
            case "startup_first_real_input_step":
            default:
            {
                const value = component ? component.startup_first_real_input_step : null;
                return value === null || value === undefined || value === "" ? Infinity : Number(value);
            }
        }
    },

    sortStartupStepsComponents(components)
    {
        const direction = inspector.startup_steps_sort_direction === "asc" ? 1 : -1;
        const sortKey = inspector.startup_steps_sort_key || "path";

        components.sort((a, b) =>
        {
            const aValue = inspector.getStartupStepsSortValue(a, sortKey);
            const bValue = inspector.getStartupStepsSortValue(b, sortKey);

            if(typeof aValue === "string" || typeof bValue === "string")
                return String(aValue).localeCompare(String(bValue)) * direction;

            const aScore = Number.isFinite(aValue) ? aValue : Infinity;
            const bScore = Number.isFinite(bValue) ? bValue : Infinity;
            if(aScore === bScore)
                return String(a && a.path ? a.path : "").localeCompare(String(b && b.path ? b.path : ""));
            return (aScore - bScore) * direction;
        });
    },

    setStartupStepsSort(sortKey)
    {
        if(inspector.startup_steps_sort_key === sortKey)
            inspector.startup_steps_sort_direction = inspector.startup_steps_sort_direction === "asc" ? "desc" : "asc";
        else
        {
            inspector.startup_steps_sort_key = sortKey;
            inspector.startup_steps_sort_direction = (sortKey === "path" || sortKey === "class") ? "asc" : "desc";
        }

        inspector.updateStartupStepsSortIndicators();
        inspector.refreshStartupStepsWindow();
    },

    updateStartupStepsSortIndicators()
    {
        const doc = inspector.getPopupWindowDocument("startup_steps_window");
        if(!doc)
            return;
        doc.querySelectorAll(".startup-steps-sort-button").forEach((button) =>
        {
            const isActive = button.dataset.sortKey === inspector.startup_steps_sort_key;
            button.classList.toggle("active", isActive);
            button.classList.toggle("sort-asc", isActive && inspector.startup_steps_sort_direction === "asc");
            button.classList.toggle("sort-desc", isActive && inspector.startup_steps_sort_direction === "desc");
            const baseLabel = button.dataset.label || button.textContent;
            button.dataset.label = baseLabel;
            button.textContent = baseLabel;
        });
    },

    renderStartupStepsWindow(data)
    {
        const doc = inspector.getPopupWindowDocument("startup_steps_window");
        if(!doc)
            return;
        const meta = doc.getElementById("startup_steps_meta");
        const rows = doc.getElementById("startup_steps_rows");
        if(!meta || !rows)
            return;

        const components = Array.isArray(data && data.components) ? [...data.components] : [];
        inspector.sortStartupStepsComponents(components);
        inspector.updateStartupStepsSortIndicators();

        meta.textContent = `tick ${Number.isFinite(data && data.tick) ? data.tick : "-"} | updated ${new Date().toLocaleTimeString()}`;

        if(components.length === 0)
        {
            rows.innerHTML = '<tr><td colspan="6">No module-start data available.</td></tr>';
            return;
        }

        rows.innerHTML = components.map((component) =>
        {
            const className = component.class || component.name || "-";
            return `<tr>
<td class="path path-column" title="${inspector.escapeHTML(component.path || "-")}">${inspector.escapeHTML(component.path || "-")}</td>
<td class="class-column">${inspector.escapeHTML(className)}</td>
<td class="number">${inspector.escapeHTML(inspector.formatModuleStart(component.module_start))}</td>
<td class="number">${inspector.escapeHTML(inspector.formatStartupStep(component.start_tick))}</td>
<td class="number">${inspector.escapeHTML(inspector.formatStartupStep(component.startup_first_real_input_step))}</td>
<td class="number">${inspector.escapeHTML(inspector.formatStartupStep(component.startup_all_real_inputs_step))}</td>
</tr>`;
        }).join("");
    },

    showStartupStepsError(message)
    {
        const doc = inspector.getPopupWindowDocument("startup_steps_window");
        if(!doc)
            return;
        const meta = doc.getElementById("startup_steps_meta");
        const rows = doc.getElementById("startup_steps_rows");
        if(meta)
        {
            meta.textContent = message;
            meta.className = "startup-steps-meta startup-steps-error";
        }
        if(rows)
            rows.innerHTML = '<tr><td colspan="6">Unable to load module-start data.</td></tr>';
    },

    refreshStartupStepsWindow()
    {
        if(!inspector.getPopupWindowDocument("startup_steps_window"))
            return;

        fetch('/startupsteps', {
            method: 'GET',
            headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id},
            cache: 'no-store'
        })
        .then((response) =>
        {
            if(!response.ok)
                throw new Error(`Startup steps request failed (${response.status})`);
            return response.json();
        })
        .then((data) =>
        {
            const doc = inspector.getPopupWindowDocument("startup_steps_window");
            const meta = doc ? doc.getElementById("startup_steps_meta") : null;
            if(meta)
                meta.className = "startup-steps-meta";
            inspector.renderStartupStepsWindow(data);
        })
        .catch((error) =>
        {
            console.log("module start update failed", error);
            inspector.showStartupStepsError("Module start update failed.");
        });
    },

    // Panel sizing and visibility management.
    escapeHTML(value)
    {
        return String(value)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#39;");
    },

    setupResizeForPanel(panel)
    {
        if(!panel)
            return;

        const computed = window.getComputedStyle(panel);
        const basis = parseInt(computed.flexBasis, 10);
        const width = parseInt(computed.width, 10);
        let defaultWidth = 300;
        if(Number.isFinite(basis) && basis > 0)
            defaultWidth = basis;
        else if(Number.isFinite(width) && width > 0)
            defaultWidth = width;
        panel.dataset.defaultWidth = String(defaultWidth);
        panel.dataset.widthCookie = panel === inspector.library ? inspector.library_width_cookie : inspector.component_width_cookie;

        let resizeHandle = panel.querySelector('.inspector-resize-handle');
        if(!resizeHandle)
        {
            resizeHandle = document.createElement('div');
            resizeHandle.className = 'inspector-resize-handle';
            panel.prepend(resizeHandle);
        }

        inspector.resetPanelWidth(panel);

        resizeHandle.addEventListener('mousedown', inspector.startResize, true);
    },

    resetPanelWidth(panel)
    {
        if(!panel)
            return;
        const cookieName = panel.dataset.widthCookie || inspector.component_width_cookie;
        const savedWidth = parseInt(getCookie(cookieName), 10);
        const maxWidth = Math.max(inspector.min_width, window.innerWidth - inspector.max_width_margin);
        const defaultWidth = parseInt(panel.dataset.defaultWidth, 10);
        const targetWidth = Number.isFinite(savedWidth) && savedWidth >= inspector.min_width
            ? savedWidth
            : (Number.isFinite(defaultWidth) && defaultWidth > 0 ? defaultWidth : 300);
        const width = Math.max(inspector.min_width, Math.min(maxWidth, targetWidth));
        panel.style.flex = `0 0 ${width}px`;
        panel.style.width = `${width}px`;
    },

    startResize(evt)
    {
        const panel = evt.currentTarget ? evt.currentTarget.parentElement : null;
        if(evt.button !== 0 || !panel)
            return;
        evt.preventDefault();
        inspector.resize_active = true;
        inspector.resize_target = panel;
        inspector.resize_start_x = evt.clientX;
        inspector.resize_start_width = panel.getBoundingClientRect().width;
        document.body.classList.add('inspector-resizing');
        document.addEventListener('mousemove', inspector.onResizeDrag, true);
        document.addEventListener('mouseup', inspector.stopResize, true);
    },

    onResizeDrag(evt)
    {
        if(!inspector.resize_active || !inspector.resize_target)
            return;
        const deltaX = inspector.resize_start_x - evt.clientX;
        const maxWidth = Math.max(inspector.min_width, window.innerWidth - inspector.max_width_margin);
        const newWidth = Math.max(inspector.min_width, Math.min(maxWidth, inspector.resize_start_width + deltaX));
        inspector.resize_target.style.flex = `0 0 ${newWidth}px`;
        inspector.resize_target.style.width = `${newWidth}px`;
        evt.preventDefault();
    },

    stopResize()
    {
        if(!inspector.resize_active)
            return;
        inspector.resize_active = false;
        document.removeEventListener('mousemove', inspector.onResizeDrag, true);
        document.removeEventListener('mouseup', inspector.stopResize, true);
        document.body.classList.remove('inspector-resizing');
        if(inspector.resize_target)
        {
            const w = Math.round(inspector.resize_target.getBoundingClientRect().width);
            const cookieName = inspector.resize_target.dataset.widthCookie || inspector.component_width_cookie;
            if(Number.isFinite(w) && w >= inspector.min_width)
                setCookie(cookieName, String(w));
        }
        inspector.resize_target = null;
    },

    hideAllPanels()
    {
        if(inspector.system)
            inspector.system.style.display = "none";
        if(inspector.component)
            inspector.component.style.display = "none";
        if(inspector.library)
            inspector.library.style.display = "none";
    },

    showPanel(panel, options = {})
    {
        if(!panel)
            return;

        inspector.hideAllPanels();
        panel.style.display = options.display || "block";

        if(options.resetWidth)
            inspector.resetPanelWidth(panel);
        if(options.renderLibrary)
            inspector.renderLibraryClassList();
        if(options.updateLibraryAddButtonState)
            inspector.updateLibraryAddButtonState();

        inspector.updateLibraryButtonState();
    },

    togglePanel(panel, options = {})
    {
        if(!panel)
            return;

        if(inspector.isPanelVisible(panel))
        {
            inspector.hideAllPanels();
            inspector.updateLibraryButtonState();
            return;
        }

        inspector.showPanel(panel, options);
    },

    toggleSystem()
    {
        inspector.togglePanel(inspector.system, {display: "block"});
    },

    toggleComponent()
    {
        inspector.togglePanel(inspector.component, {display: "block", resetWidth: true});
    },

    toggleLibrary()
    {
        inspector.togglePanel(inspector.library, {
            display: "flex",
            resetWidth: true,
            renderLibrary: true,
            updateLibraryAddButtonState: true
        });
    },

    showComponent()
    {
        inspector.showPanel(inspector.component, {display: "block", resetWidth: true});
    },

    showLibrary()
    {
        inspector.showPanel(inspector.library, {
            display: "flex",
            resetWidth: true,
            renderLibrary: true,
            updateLibraryAddButtonState: true
        });
    },

    showLibraryForClass(className)
    {
        if(!className)
            return;
        inspector.selected_library_class = className;
        inspector.showLibrary();
    },

    updateLibraryButtonState()
    {
        const button = document.getElementById("library_inspector_button");
        if(!button || !inspector.library)
            return;
        const open = window.getComputedStyle(inspector.library, null).display !== 'none';
        button.setAttribute("aria-pressed", open ? "true" : "false");
    },

    updateLibraryAddButtonState()
    {
        const button = document.getElementById("library_add_button");
        if(!button)
            return;
        const canAdd = !!(main && main.edit_mode && inspector.selected_library_class);
        button.disabled = !canAdd;
    },

    isPanelVisible(panel)
    {
        return !!(panel && window.getComputedStyle(panel, null).display !== 'none');
    },

    isAnyInspectorOpen()
    {
        return inspector.isPanelVisible(inspector.system)
            || inspector.isPanelVisible(inspector.component)
            || inspector.isPanelVisible(inspector.library);
    },

    revealSelectionPanel(shouldReveal=false)
    {
        if(!shouldReveal || !inspector.isAnyInspectorOpen() || inspector.isPanelVisible(inspector.component))
            return;
        inspector.showComponent();
    },

    updateSelectionPanelWidth()
    {
        const signature = selector.selected_foreground.length > 0
            ? selector.selected_foreground.join("|")
            : "";
        if(signature !== inspector.last_selected_signature)
            inspector.last_selected_signature = signature;
        inspector.resetPanelWidth(inspector.component);
    },

    // Library browser and markdown/documentation rendering.
    renderLibraryClassList()
    {
        if(!inspector.libraryClassList)
            return;

        inspector.libraryClassList.innerHTML = "";

        const classes = Array.isArray(network.classes) ? network.classes : [];
        if(classes.length === 0)
        {
            const emptyState = document.createElement("div");
            emptyState.className = "library-inspector-placeholder";
            emptyState.textContent = "No module classes available yet.";
            inspector.libraryClassList.appendChild(emptyState);
            if(inspector.libraryClassDetails && !inspector.selected_library_class)
                inspector.libraryClassDetails.textContent = "Select a module class to view details here later.";
            return;
        }

        if(!inspector.selected_library_class || !classes.includes(inspector.selected_library_class))
            inspector.selected_library_class = classes[0];
        inspector.updateLibraryAddButtonState();

        for(const className of classes)
        {
            const button = document.createElement("button");
            button.type = "button";
            button.className = "library-class-item";
            if(className === inspector.selected_library_class)
                button.classList.add("is-selected");
            button.setAttribute("role", "option");
            button.setAttribute("aria-selected", className === inspector.selected_library_class ? "true" : "false");
            button.textContent = className;
            button.addEventListener("click", function()
            {
                inspector.selectLibraryClass(className);
            });
            inspector.libraryClassList.appendChild(button);
        }

        inspector.revealSelectedLibraryClass();
        inspector.updateLibraryClassDetails();
    },

    revealSelectedLibraryClass()
    {
        if(!inspector.libraryClassList || !inspector.selected_library_class)
            return;

        const selectedButton = inspector.libraryClassList.querySelector('.library-class-item.is-selected');
        if(!selectedButton)
            return;

        selectedButton.scrollIntoView({block: "nearest"});
    },

    selectLibraryClass(className)
    {
        inspector.selected_library_class = className;
        inspector.updateLibraryAddButtonState();
        inspector.renderLibraryClassList();
    },

    addSelectedLibraryClass()
    {
        if(!main.edit_mode || !inspector.selected_library_class)
            return;
        const fullName = main.newModule(inspector.selected_library_class);
        if(fullName)
            selector.selectItems([fullName], selector.selected_background, false, false, true);
    },

    escapeHtml(text)
    {
        return String(text)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#39;");
    },

    renderInlineMarkdown(text, options = {})
    {
        let html = inspector.escapeHtml(text);
        html = html.replace(/&lt;br\s*\/?&gt;/gi, "<br>");
        html = html.replace(/`([^`]+)`/g, "<code>$1</code>");
        html = html.replace(/!\[([^\]]*)\]\(([^)\s]+)\)/g, (_match, alt, url) =>
        {
            const src = inspector.resolveMarkdownAssetUrl(url, options.className);
            return `<img src="${src}" alt="${inspector.escapeHtml(alt)}" loading="lazy">`;
        });
        html = html.replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, '<a href="$2" target="_blank" rel="noopener noreferrer">$1</a>');
        html = html.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>");
        html = html.replace(/\*([^*]+)\*/g, "<em>$1</em>");
        return html;
    },

    resolveMarkdownAssetUrl(url, className = "")
    {
        const value = String(url || "").trim();
        if(!value)
            return "";
        if(/^(?:[a-z]+:)?\/\//i.test(value) || value.startsWith("data:") || value.startsWith("/"))
            return value;
        if(className && network.classinfo && network.classinfo[className] && network.classinfo[className].path)
        {
            const classPath = String(network.classinfo[className].path).replace(/\\/g, "/");
            const sourceModules = "/Source/Modules/";
            const relativeIndex = classPath.indexOf(sourceModules);
            if(relativeIndex !== -1)
            {
                const moduleRelativePath = classPath.slice(relativeIndex + "/Source/".length);
                return `/../${moduleRelativePath}/${encodeURIComponent(value)}`;
            }
        }
        return value;
    },

    renderMarkdown(text, options = {})
    {
        const lines = String(text || "").replace(/\r\n/g, "\n").split("\n");
        const blocks = [];
        let paragraph = [];
        let listItems = [];
        let codeLines = [];
        let inCodeBlock = false;
        let tableRows = [];

        const isTableLine = function(line)
        {
            return /\|/.test(line);
        };

        const isTableDivider = function(line)
        {
            const normalized = line.trim().replace(/^\||\|$/g, "").trim();
            if(normalized === "")
                return false;
            return normalized.split("|").every((cell) => /^:?-{3,}:?$/.test(cell.trim()));
        };

        const splitTableRow = function(line)
        {
            return line.trim().replace(/^\||\|$/g, "").split("|").map((cell) => cell.trim());
        };

        const flushParagraph = function()
        {
            if(paragraph.length === 0)
                return;
            blocks.push(`<p>${inspector.renderInlineMarkdown(paragraph.join(" "), options)}</p>`);
            paragraph = [];
        };

        const flushList = function()
        {
            if(listItems.length === 0)
                return;
            blocks.push(`<ul>${listItems.map((item) => `<li>${inspector.renderInlineMarkdown(item, options)}</li>`).join("")}</ul>`);
            listItems = [];
        };

        const flushTable = function()
        {
            if(tableRows.length < 2)
            {
                if(tableRows.length === 1)
                    paragraph.push(tableRows[0].join(" | "));
                tableRows = [];
                return;
            }

            const header = tableRows[0];
            const bodyRows = tableRows.slice(2);
            const thead = `<thead><tr>${header.map((cell) => `<th>${inspector.renderInlineMarkdown(cell, options)}</th>`).join("")}</tr></thead>`;
            const tbody = bodyRows.length > 0
                ? `<tbody>${bodyRows.map((row) => `<tr>${row.map((cell) => `<td>${inspector.renderInlineMarkdown(cell, options)}</td>`).join("")}</tr>`).join("")}</tbody>`
                : "";
            blocks.push(`<table>${thead}${tbody}</table>`);
            tableRows = [];
        };

        const flushCodeBlock = function()
        {
            if(codeLines.length === 0)
                return;
            blocks.push(`<pre><code>${inspector.escapeHtml(codeLines.join("\n"))}</code></pre>`);
            codeLines = [];
        };

        for(const line of lines)
        {
            if(line.trim().startsWith("```"))
            {
                flushParagraph();
                flushList();
                flushTable();
                if(inCodeBlock)
                {
                    flushCodeBlock();
                    inCodeBlock = false;
                }
                else
                {
                    inCodeBlock = true;
                }
                continue;
            }

            if(inCodeBlock)
            {
                codeLines.push(line);
                continue;
            }

            const trimmed = line.trim();
            if(trimmed === "")
            {
                flushParagraph();
                flushList();
                flushTable();
                continue;
            }

            if(isTableLine(line))
            {
                const row = splitTableRow(line);
                if(tableRows.length === 0)
                {
                    flushParagraph();
                    flushList();
                    tableRows.push(row);
                    continue;
                }
                if(tableRows.length === 1 && isTableDivider(line))
                {
                    tableRows.push(row);
                    continue;
                }
                if(tableRows.length >= 2)
                {
                    tableRows.push(row);
                    continue;
                }
            }
            else
            {
                flushTable();
            }

            const heading = trimmed.match(/^(#{1,6})\s+(.*)$/);
            if(heading)
            {
                flushParagraph();
                flushList();
                flushTable();
                const level = Math.min(6, heading[1].length);
                blocks.push(`<h${level}>${inspector.renderInlineMarkdown(heading[2], options)}</h${level}>`);
                continue;
            }

            const listItem = trimmed.match(/^[-*]\s+(.*)$/);
            if(listItem)
            {
                flushParagraph();
                listItems.push(listItem[1]);
                continue;
            }

            flushList();
            paragraph.push(trimmed);
        }

        flushParagraph();
        flushList();
        flushTable();
        flushCodeBlock();

        if(blocks.length === 0)
            return "<p></p>";
        return blocks.join("");
    },

    updateLibraryClassDetails()
    {
        if(!inspector.libraryClassDetails)
            return;
        if(!inspector.selected_library_class)
        {
            inspector.libraryClassDetails.innerHTML = "<p>Select a module class to view details here later.</p>";
            return;
        }
        const className = inspector.selected_library_class;
        if(Object.prototype.hasOwnProperty.call(inspector.library_readme_cache, className))
        {
            inspector.libraryClassDetails.innerHTML = inspector.renderMarkdown(inspector.library_readme_cache[className], {className});
            return;
        }

        inspector.pending_library_readme_class = className;
        inspector.libraryClassDetails.innerHTML = `<p>Loading ReadMe.md for ${inspector.escapeHtml(className)}...</p>`;
        fetch(`/classreadme?class=${encodeURIComponent(className)}`, {
            method: "GET",
            headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}
        })
        .then(response => {
            if(!response.ok)
                throw new Error("HTTP error " + response.status);
            return response.text();
        })
        .then(text => {
            inspector.library_readme_cache[className] = text;
            if(inspector.pending_library_readme_class === className && inspector.selected_library_class === className)
                inspector.libraryClassDetails.innerHTML = inspector.renderMarkdown(text, {className});
        })
        .catch(() => {
            const fallback = `Could not load ReadMe.md for ${className}.`;
            inspector.library_readme_cache[className] = fallback;
            if(inspector.pending_library_readme_class === className && inspector.selected_library_class === className)
                inspector.libraryClassDetails.innerHTML = `<p>${inspector.escapeHtml(fallback)}</p>`;
        });
    },

    // Inspector table and row-building helpers.
    hideSubviews()
    {
        Object.entries(inspector.subview).forEach(([key, value]) => { value.style.display = 'none'; });
    },

    setTable(table)
    {
        current_t_body = table.tBodies[0];
        current_t_body.innerHTML = "";
    },

    addTableRow(label, content="")
    {
        const row = current_t_body.insertRow(-1);
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        cell1.innerText = label;
        cell2.innerHTML = content;
        const firstElement = cell2.firstElementChild;
        if(firstElement && firstElement.tagName == "SELECT")
        {
            cell2.classList.add("inspector-menu-cell");
            cell2.style.paddingLeft = "0";
        }
        return cell2.firstElementChild;
    },

    focusEditableField(label)
    {
        if(!inspector.subview.table)
            return false;
        const rows = inspector.subview.table.querySelectorAll("tbody tr");
        for(const row of rows)
        {
            const cells = row.cells || [];
            if(cells.length < 2)
                continue;
            if((cells[0].innerText || "").trim() !== label)
                continue;
            const target = cells[1];
            if(!target || target.contentEditable !== "true")
                continue;
            target.focus();
            return true;
        }
        return false;
    },

    addHeader(header)
    {
        current_t_body.innerHTML += `<tr><td colspan='2' class='header'>${header}</td></tr>`;
    },

    addAttributeValue(attribute, value)
    {
        this.addTableRow(attribute,value);
    },

    addMenu(name, value, opts, attribute)
    {
        let s = '<select name="'+name+'" oninput="this">';
        for(let j in opts)
        {
            const val= opts[j];
            if(opts[j] == value)
                s += '<option value="'+val+'" selected >'+opts[j]+'</option>';
            else
                s += '<option value="'+val+'">'+opts[j]+'</option>';
        }
        s += '</select>';

        return inspector.addTableRow(name, s);
    },

    addRuntimeLogLevelControl(item)
    {
        const currentLevel = Number.isFinite(Number(item.log_level)) ? Number(item.log_level) : 0;
        const selectedLabel = logLevelOptions[currentLevel] || logLevelOptions[0];
        const logLevelMenu = inspector.addMenu("log_level", selectedLabel, logLevelOptions);
        const applyLogLevel = function()
        {
            item.log_level = logLevelOptions.indexOf(this.value);
            selector.setLogLevel(item.log_level);
        };
        logLevelMenu.addEventListener("input", applyLogLevel);
        logLevelMenu.addEventListener("change", applyLogLevel);
    },

    createTemplate(component)
    {
        let t = [];
        for(let [a,v] of Object.entries(component))
        {
            if(!["name","class"].includes(a) && a[0]!='_')
                t.push({'name':a,'control':'textedit', 'type':'source'});
        }
        return t;
    },

    checkValueForType(value, type)
    {
        return true;

        if(type=="string")
            return true;

        if(value[0] === '@')
            return true;

        return false;
    },

    parseValueForType(value, type, fallbackValue)
    {
        if(type == "int")
        {
            const parsed = parseInt(value);
            return Number.isFinite(parsed) ? parsed : fallbackValue;
        }
        if(type == "float" || type == "number")
        {
            const parsed = parseFloat(value);
            return Number.isFinite(parsed) ? parsed : fallbackValue;
        }
        return value;
    },

    syncNotifiedParameter(item, p, nextValue, options = {})
    {
        const shouldNotify = options.notify !== false;
        item[p.name] = nextValue;
        if(inspector.notify && inspector.notify.parameters)
            inspector.notify.parameters[p.name] = item[p.name];
        if(shouldNotify && inspector.notify)
            inspector.notify.parameterChangeNotification(p);
        return item[p.name];
    },

    normalizeTextEditValue(target)
    {
        return String(target?.innerText ?? target?.textContent ?? "")
            .replace(/\r\n/g, "\n")
            .replace(/\r/g, "\n")
            .replace(/\u200B/g, "");
    },

    acreateHeaderRow(item, template)
    {
        const row = current_t_body.insertRow(-1);
        const cell1 = row.insertCell(0);
        cell1.innerText = template.name;
    },

    createTextEditRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const value = item[p.name];
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        const hasDefaultPlaceholder = !!(p && p.default !== undefined && p.default !== null && p.default !== "");
        const updateDefaultPlaceholderState = (targetCell, isEmpty) =>
        {
            if(!hasDefaultPlaceholder)
            {
                targetCell.removeAttribute("data-placeholder");
                targetCell.classList.remove("inspector-default-placeholder");
                return;
            }
            targetCell.setAttribute("data-placeholder", String(p.default));
            if(isEmpty)
                targetCell.classList.add("inspector-default-placeholder");
            else
                targetCell.classList.remove("inspector-default-placeholder");
        };

        cell1.innerText = p.name;
        const hasValue = !(value === undefined || value === null || value === "");
        cell2.textContent = hasValue ? String(value) : "";
        cell2.setAttribute('class', p.type + ' textedit');
        updateDefaultPlaceholderState(cell2, !hasValue);
        cell2.addEventListener("paste", function(e) 
        {
            e.preventDefault();
            const text = e.clipboardData.getData("text/plain");
            document.execCommand("insertHTML", false, text);
        });

        cell2.contentEditable = true;
        const commitOnInput = !(
            p.name === "name" ||
            p.name === "grid_spacing" ||
            (inspector.item && inspector.item._tag == "connection")
        );
        const commitTextEditValue = function(evt)
        {
            let newValue = inspector.normalizeTextEditValue(evt.target);
            if(p.type != "string")
                newValue = newValue.replace(/\n/g, "");
            if(!inspector.checkValueForType(newValue, p.type))
                return;
            inspector.syncNotifiedParameter(item, p, inspector.parseValueForType(newValue, p.type, item[p.name]));
        };
        cell2.addEventListener("keypress", function(evt) 
        {
            if(evt.keyCode == 13 && p.type != "string")
            {
                evt.target.blur();
                evt.preventDefault();
                return;
            }
        });

        cell2.addEventListener("input", function(evt)
        {
            updateDefaultPlaceholderState(evt.target, inspector.normalizeTextEditValue(evt.target).trim() === "");
            if(commitOnInput)
                commitTextEditValue(evt);
        });

        cell2.addEventListener("blur", function(evt) 
        {
            commitTextEditValue(evt);
            updateDefaultPlaceholderState(evt.target, inspector.normalizeTextEditValue(evt.target).trim() === "");
        });
    },

    createRangeEditorRows(item, p)
    {
        const currentValue = typeof item[p.name] === "string" ? item[p.name] : "";
        const rows = parseBracketRangeString(currentValue);
        const displayRows = rows.slice();

        const commitRangeRows = function()
        {
            const serialized = serializeBracketRangeParts(displayRows);
            inspector.syncNotifiedParameter(item, p, serialized);
        };

        const buildRangePlaceholder = function(axis)
        {
            if(axis == "start")
                return "start";
            if(axis == "end")
                return "end";
            return "step";
        };

        for(let i = 0; i < displayRows.length; i++)
        {
            const rangeRow = displayRows[i];
            const row = current_t_body.insertRow(-1);
            const labelCell = row.insertCell(0);
            const editorCell = row.insertCell(1);

            labelCell.innerText = `${p.label || p.name} [${i+1}]`;
            editorCell.className = `${p.type || "range"} textedit range-editor`;
            editorCell.classList.add("inspector-removable-value-cell");

            const wrapper = document.createElement("div");
            wrapper.className = "range-editor-fields";

            const fields = [
                {key:"start", inputMode:"numeric"},
                {key:"end", inputMode:"numeric"},
                {key:"step", inputMode:"numeric"}
            ];
            const rowInputs = [];

            for(const field of fields)
            {
                const input = document.createElement("input");
                input.type = "text";
                input.value = rangeRow[field.key] || "";
                input.placeholder = buildRangePlaceholder(field.key);
                input.setAttribute("aria-label", `${p.name} ${i+1} ${field.key}`);
                input.className = "range-editor-input";
                if(field.inputMode)
                    input.inputMode = field.inputMode;
                input.style.width = "100%";
                const updateFieldValue = function(evt)
                {
                    rangeRow[field.key] = evt.target.value;
                    inspector.syncNotifiedParameter(item, p, serializeBracketRangeParts(displayRows), {notify: false});
                };
                const commitFieldValue = function(evt)
                {
                    updateFieldValue(evt);
                    commitRangeRows();
                };
                input.addEventListener("input", updateFieldValue);
                input.addEventListener("change", updateFieldValue);
                input.addEventListener("blur", function(evt)
                {
                    updateFieldValue(evt);
                    const nextTarget = evt.relatedTarget;
                    if(nextTarget && wrapper.contains(nextTarget))
                        return;
                    commitRangeRows();
                });
                input.addEventListener("keydown", function(evt)
                {
                    if(evt.key === "Enter")
                    {
                        evt.preventDefault();
                        const currentIndex = rowInputs.indexOf(evt.target);
                        const nextInput = currentIndex >= 0 ? rowInputs[currentIndex + 1] : null;
                        if(nextInput)
                            nextInput.focus();
                        else
                            evt.target.blur();
                    }
                });
                rowInputs.push(input);
                wrapper.appendChild(input);

                if(
                    item._range_editor_focus &&
                    item._range_editor_focus.name === p.name &&
                    item._range_editor_focus.index === i &&
                    item._range_editor_focus.key === field.key
                )
                {
                    const focusTarget = input;
                    setTimeout(function()
                    {
                        focusTarget.focus();
                        focusTarget.select();
                    }, 0);
                    delete item._range_editor_focus;
                }
            }

            editorCell.appendChild(wrapper);

            const removeButton = document.createElement("button");
            removeButton.type = "button";
            removeButton.className = "inspector-minus-button";
            removeButton.setAttribute("aria-label", "Remove range");
            removeButton.setAttribute("contenteditable", "false");
            removeButton.innerHTML = `
                <svg viewBox="0 0 24 24" width="16" height="16" aria-hidden="true" focusable="false">
                    <path d="M18 6L17.1991 18.0129C17.129 19.065 17.0939 19.5911 16.8667 19.99C16.6666 20.3412 16.3648 20.6235 16.0011 20.7998C15.588 21 15.0607 21 14.0062 21H9.99377C8.93927 21 8.41202 21 7.99889 20.7998C7.63517 20.6235 7.33339 20.3412 7.13332 19.99C6.90607 19.5911 6.871 19.065 6.80086 18.0129L6 6M4 6H20M16 6L15.7294 5.18807C15.4671 4.40125 15.3359 4.00784 15.0927 3.71698C14.8779 3.46013 14.6021 3.26132 14.2905 3.13878C13.9376 3 13.523 3 12.6936 3H11.3064C10.477 3 10.0624 3 9.70951 3.13878C9.39792 3.26132 9.12208 3.46013 8.90729 3.71698C8.66405 4.00784 8.53292 4.40125 8.27064 5.18807L8 6M14 10V17M10 10V17" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path>
                </svg>
            `;
            removeButton.addEventListener("mousedown", function(evt)
            {
                evt.preventDefault();
            });
            removeButton.addEventListener("click", function(evt)
            {
                evt.preventDefault();
                evt.stopPropagation();
                const updatedRows = parseBracketRangeString(item[p.name]);
                if(i < updatedRows.length)
                    updatedRows.splice(i, 1);
                inspector.syncNotifiedParameter(item, p, serializeBracketRangeParts(updatedRows));
            });
            editorCell.appendChild(removeButton);
        }

        const actionRow = current_t_body.insertRow(-1);
        const actionCellLeft = actionRow.insertCell(0);
        const actionCellRight = actionRow.insertCell(1);
        actionCellLeft.innerText = displayRows.length === 0 ? (p.label || p.name) : "";
        actionCellRight.className = "inspector-action-cell";
        actionCellRight.style.textAlign = "right";
        actionCellRight.innerHTML = '<button type="button" class="inspector-plus-button" aria-label="Add range">+</button>';

        const plusButton = actionCellRight.querySelector(".inspector-plus-button");
        if(plusButton)
        {
            const requestAddRangeRow = function()
            {
                const updatedRows = parseBracketRangeString(item[p.name]);
                updatedRows.push({start:"", end:"", step:""});
                inspector.syncNotifiedParameter(item, p, serializeBracketRangeParts(updatedRows), {notify: false});
                item._range_editor_focus = {name: p.name, index: updatedRows.length - 1, key: "start"};
                if(inspector.notify)
                    inspector.notify.parameterChangeNotification(p);
            };
            plusButton.addEventListener("mousedown", function(evt)
            {
                evt.preventDefault();
                evt.stopPropagation();
                const active = document.activeElement;
                if(active && active.closest && active.closest("#component_inspector") && typeof active.blur == "function")
                {
                    active.blur();
                    setTimeout(requestAddRangeRow, 0);
                }
                else
                    requestAddRangeRow();
            });
        }
    },

    createMenuRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.innerText = p.name;
        cell2.className = "inspector-menu-cell";
        cell2.style.paddingLeft = "0";

        let opts = p.options.split(',').map(o=>o.trim());
                        
        let s = '<select name="'+p.name+'">';
        for(let j in opts)
        {
            let optionValue = p.type == 'int' ? j : opts[j];
            let optionLabel = opts[j];
            let optionStyle = "";
            if(p.option_labels && Object.prototype.hasOwnProperty.call(p.option_labels, optionValue))
                optionLabel = p.option_labels[optionValue];
            if(p.option_styles && Object.prototype.hasOwnProperty.call(p.option_styles, optionValue))
                optionStyle = ` style="${p.option_styles[optionValue]}"`;
            if(opts[j] == item[p.name])
                s += '<option value="'+optionValue+'" selected'+optionStyle+'>'+optionLabel+'</option>';
            else
                s += '<option value="'+optionValue+'"'+optionStyle+'>'+optionLabel+'</option>';
        }
        s += '</select>';
        cell2.innerHTML= s;
        cell2.addEventListener("input", function(evt) { 
                inspector.syncNotifiedParameter(item, p, evt.target.value.trim());
            });
    },

    createUIColorRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.innerText = p.name;
        cell2.className = "ui-color-cell";
        cell2.style.paddingLeft = "0";

        const colorName = item[p.name] || "black";
        const wrapper = document.createElement("div");
        wrapper.className = "inspector-ui-color-display";

        const swatch = document.createElement("span");
        swatch.className = "inspector-ui-color-swatch";
        swatch.style.background = inspectorUIColorSwatches[colorName] || inspectorUIColorSwatches.black;
        swatch.setAttribute("aria-label", colorName);
        swatch.title = colorName;
        swatch.tabIndex = 0;
        const openColorMenu = function()
        {
            let fullName = null;
            if(selector.selected_foreground.length == 1)
                fullName = selector.selected_foreground[0];
            else if(inspector.item && inspector.item._tag == "group" && selector.selected_foreground.length == 0)
                fullName = selector.selected_background;
            else if(inspector.item && inspector.item._tag == "connection" && selector.selected_connection)
                fullName = selector.selected_connection;
            if(!fullName)
                return;
            const rect = swatch.getBoundingClientRect();
            main.showComponentColorMenuAt(rect.right + 2, rect.top, fullName, "color-only", p.name);
        };
        swatch.addEventListener("click", function(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
            openColorMenu();
        });
        swatch.addEventListener("keydown", function(evt)
        {
            if(evt.key === "Enter" || evt.key === " ")
            {
                evt.preventDefault();
                evt.stopPropagation();
                openColorMenu();
            }
        });
        wrapper.appendChild(swatch);

        const label = document.createElement("span");
        label.className = "inspector-ui-color-label";
        label.textContent = colorName;
        wrapper.appendChild(label);

        cell2.appendChild(wrapper);
    },

    createColorRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.innerText = p.name;
        cell2.className = "inspector-rgb-color-cell";
        cell2.style.paddingLeft = "0";

        const wrapper = document.createElement("div");
        wrapper.className = "inspector-rgb-color-editor";

        const previewRow = document.createElement("div");
        previewRow.className = "inspector-rgb-color-preview-row";

        const swatch = document.createElement("span");
        swatch.className = "inspector-rgb-color-swatch";
        previewRow.appendChild(swatch);

        const valueLabel = document.createElement("span");
        valueLabel.className = "inspector-rgb-color-value";
        previewRow.appendChild(valueLabel);

        wrapper.appendChild(previewRow);

        const sliderContainer = document.createElement("div");
        sliderContainer.className = "inspector-rgb-slider-list";

        const channels = [
            { key: "r", label: "R" },
            { key: "g", label: "G" },
            { key: "b", label: "B" }
        ];
        const sliders = {};
        const channelValues = parseInspectorRGBColor(item[p.name]);

        const commitColor = function()
        {
            const nextValue = formatInspectorRGBColor(sliders.r.value, sliders.g.value, sliders.b.value);
            inspector.syncNotifiedParameter(item, p, nextValue);
            swatch.style.backgroundColor = nextValue;
            valueLabel.textContent = nextValue;
        };

        for(const channel of channels)
        {
            const sliderRow = document.createElement("label");
            sliderRow.className = "inspector-rgb-slider-row";

            const channelLabel = document.createElement("span");
            channelLabel.className = "inspector-rgb-slider-label";
            channelLabel.textContent = channel.label;
            sliderRow.appendChild(channelLabel);

            const slider = document.createElement("input");
            slider.type = "range";
            slider.min = "0";
            slider.max = "255";
            slider.step = "1";
            slider.value = String(channelValues[channel.key]);
            slider.className = "inspector-rgb-slider-input";
            slider.addEventListener("input", commitColor);
            slider.addEventListener("change", commitColor);
            sliderRow.appendChild(slider);

            sliders[channel.key] = slider;
            sliderContainer.appendChild(sliderRow);
        }

        wrapper.appendChild(sliderContainer);
        cell2.appendChild(wrapper);
        commitColor();
    },

    createCheckBoxRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const value = item[p.name];
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.innerText = p.name;
        cell2.className = "inspector-checkbox-cell";
        cell2.style.paddingLeft = "0";
        if(value)
            cell2.innerHTML= '<input type="checkbox" checked />';
        else
            cell2.innerHTML= '<input type="checkbox" />';
        const commitCheckBoxValue = function(evt)
        {
            inspector.syncNotifiedParameter(item, p, evt.target.checked);
        };
        cell2.addEventListener("input", commitCheckBoxValue);
        cell2.addEventListener("change", commitCheckBoxValue);

    },

    createSliderRow(item, p)
    {
        this.createTextEditRow(item, p);
    },

    getRowBuilder(control)
    {
        switch(control)
        {
            case "header":
                return inspector.acreateHeaderRow;
            case "textedit":
                return inspector.createTextEditRow;
            case "range-editor":
                return inspector.createRangeEditorRows;
            case "color":
                return inspector.createColorRow;
            case "ui_color":
                return inspector.createUIColorRow;
            case "menu":
                return inspector.createMenuRow;
            case "checkbox":
                return inspector.createCheckBoxRow;
            case "slider":
                return inspector.createSliderRow;
            default:
                return null;
        }
    },

    // Input controls and editable inspector rows.
    addDataRows(item, template, notify=null)
    {
        try{
            inspector.item = item;
            inspector.template = template;
            inspector.notify = notify;
    
            for(let p of inspector.template)
            {
                if(p.control == "ui_color")
                {
                    p.type = p.type || "source";
                    p.options = inspectorUIColorOptions;
                    p.option_labels = inspectorUIColorLabels;
                    p.option_styles = inspectorUIColorStyles;
                }

                if(p.control == undefined)
                {
                    if(p.type=="bool")
                        p.control = "checkbox";
                    else if(p.options)
                        p.control = "menu";
                    else
                        p.control = "textedit";
                }

                const rowBuilder = inspector.getRowBuilder(p.control);
                if(rowBuilder)
                    rowBuilder.call(this, item, p);
            }
        }
        catch(err)
        {
            console.log("addDataRows: "+err);
        }
    },

    checkRangeSyntax(s)
    {
        if(!s)
            return "";
        if(s === "")
            return "";
        if(s[0]!="[")
        {
            alert("Range must start with [");
            return "";
        }
        if(s[s.length-1]!="]")
        {
            alert("Range must end with ]");
            return "";
        }
        return s;
    },

    // Data application and selection-specific inspector rendering.
    parameterChangeNotification(p)
    {
        const isTopGroupBackground = (
            inspector.item &&
            inspector.item._tag == "group" &&
            selector.selected_foreground.length == 0 &&
            selector.selected_background == network.network.name
        );

        if(isTopGroupBackground && p && p.name == "tick_duration")
        {
            const parsed = parseFloat(inspector.item.tick_duration);
            const tickDuration = Number.isFinite(parsed) ? parsed : 0;
            inspector.item.tick_duration = tickDuration;
            controller.tick_duration = tickDuration;
            return;
        }

        if(
            inspector.item &&
            inspector.item._tag == "group" &&
            selector.selected_foreground.length == 0 &&
            p &&
            p.name == "grid_spacing"
        )
        {
            controller.setTainted(true, "parameterChangeNotification grid_spacing");
            main.applyBackgroundGridSpacing();
            return;
        }

        if(
            inspector.item &&
            inspector.item._tag == "group" &&
            selector.selected_foreground.length == 0 &&
            p &&
            p.name == "auto_routing"
        )
        {
            main.updateAutoRoutingButtonState();
            main.addConnections();
        }

        if(
            inspector.item &&
            ["module", "group", "input", "output"].includes(inspector.item._tag) &&
            p &&
            p.name == "color"
        )
        {
            controller.setTainted(true, "parameterChangeNotification color");
            main.selectItem(selector.selected_foreground, selector.selected_background);
            inspector.showInspectorForSelection();
            return;
        }

        if(inspector.item._tag == "connection")
        {
            this.item.source = getStringUpToBracket(this.item.source)+this.checkRangeSyntax(this.item.source_range);
            this.item.target = getStringUpToBracket(this.item.target)+this.checkRangeSyntax(this.item.target_range);
            selector.selectConnection(selector.selected_connection);
        }
        else if(inspector.item._tag == "group" && selector.selected_foreground.length == 0)
            {
                let old_name = selector.selected_background;
                let new_name = changeNameInPath(selector.selected_background, inspector.item.name);
                if(new_name != old_name)
                {
                    network.dict[new_name] = inspector.item;
                    delete network.dict[old_name];
                    selector.selectItems([], new_name);
                }
        }
        else if (inspector.item._tag == "input")
        {
            let old_name = selector.selected_foreground[0];
            let new_name = selector.selected_background+'.'+inspector.item.name;
            let group = selector.selected_background;

            network.renameInput(group, old_name, new_name);
            selector.selectItems([new_name], null);
         }
        else if (inspector.item._tag == "output")
        {
            let old_name = selector.selected_foreground[0];
            let new_name = selector.selected_background+'.'+inspector.item.name;
            let group = selector.selected_background;

            network.renameOutput(group, old_name, new_name);
            selector.selectItems([new_name], null);
         }
        else
        {
            let old_name = selector.selected_foreground[0];
            let new_name = selector.selected_background+'.'+inspector.item.name;
            let group = selector.selected_background;

            if(new_name != old_name)
            {
                network.renameGroupOrModule(group, old_name, new_name);
                selector.selectItems([new_name], null);
            }
        }
        network.rebuildDict();
        nav.populate();
    },

    showGroupBackground(bg)
    {
        inspector.hideSubviews();
        let item = network.dict[bg];
        const isTopGroup = (bg == network.network.name);
        const rowTemplate = [];
        const readOnlyRows = [];
        const countRowKeys = ["groups", "modules", "connections", "widgets", "inputs", "outputs"];
        const countRows = [];
        const allAttributes = Object.keys(item || {});
        const hiddenTopGroupAttributes = new Set(["webui_port", "info", "stop"]);
        const priorityRowTemplates = new Map();
        for(const key of allAttributes)
        {
            if(key == "_tag" || key.startsWith("_"))
                continue;
            if(key == "parameters")
                continue;
            if(isTopGroup && hiddenTopGroupAttributes.has(key))
                continue;
            if(key == "name" || key == "tick_duration" || key == "log_level")
                continue;
            const value = item[key];
            if(countRowKeys.includes(key))
            {
                countRows.push({ key, value: (item[key] || []).length });
                continue;
            }
            if(Array.isArray(value))
                continue;
            if(key == "color")
                continue;
            if(key == "auto_routing")
            {
                priorityRowTemplates.set(key, {'name': key, 'control':'checkbox', 'type':'bool'});
                continue;
            }
            if(typeof value == "number")
                rowTemplate.push({'name': key, 'control':'textedit', 'type': Number.isInteger(value) ? 'int' : 'float'});
            else if(typeof value == "boolean")
                rowTemplate.push({'name': key, 'control':'checkbox', 'type':'bool'});
            else if(typeof value == "string" || value === undefined || value === null)
                rowTemplate.push({'name': key, 'control':'textedit', 'type':'source'});
            else
                readOnlyRows.push({ key, value });
        }
        const orderedPriorityRows = [];
        if(priorityRowTemplates.has("auto_routing"))
            orderedPriorityRows.push(priorityRowTemplates.get("auto_routing"));
        if(!rowTemplate.some((row) => row.name == "proxy"))
            orderedPriorityRows.push({'name': 'proxy', 'control':'textedit', 'type':'source'});
        rowTemplate.unshift(...orderedPriorityRows);
        for(const key of countRowKeys)
            if(!countRows.some((r) => r.key == key))
                countRows.push({ key, value: (item[key] || []).length });

        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';

        inspector.addHeader(isTopGroup ? "Top Group" : "Group");
        if(main.edit_mode)
        {
            const template = [{'name':'name', 'control':'textedit', 'type':'source'}];
            if(isTopGroup)
            {
                if(item.tick_duration === undefined)
                    item.tick_duration = controller.tick_duration || 0;
                template.push({'name':'tick_duration', 'control':'textedit', 'type':'float'});
            }
            if(!isTopGroup)
            {
                if(!item.color)
                    item.color = "black";
                template.push({
                    'name':'color',
                    'control':'ui_color',
                    'type':'source'
                });
            }
            inspector.addDataRows(item, template, inspector);
            const logLevelMenu = inspector.addMenu("log_level", logLevelOptions[item.log_level ?? 0], logLevelOptions);
            const applyGroupLogLevel = function()
            {
                item.log_level = logLevelOptions.indexOf(this.value);
                selector.setLogLevel(item.log_level);
            };
            logLevelMenu.addEventListener("input", applyGroupLogLevel);
            logLevelMenu.addEventListener("change", applyGroupLogLevel);
            if(rowTemplate.length > 0)
            {
                const firstCustomRowIndex = current_t_body.rows.length;
                inspector.addDataRows(item, rowTemplate, inspector);
                for(let i = 0; i < rowTemplate.length; i++)
                {
                    const attribute = rowTemplate[i];
                    const row = current_t_body.rows[firstCustomRowIndex + i];
                    if(!row || row.cells.length < 2)
                        continue;
                    if(attribute.name == "auto_routing" || attribute.name == "color")
                        continue;
                    const valueCell = row.cells[1];
                    valueCell.classList.add("inspector-removable-value-cell");
                    const removeButton = document.createElement("button");
                    removeButton.type = "button";
                    removeButton.className = "inspector-minus-button";
                    removeButton.setAttribute("aria-label", "Remove attribute");
                    removeButton.setAttribute("contenteditable", "false");
                    removeButton.innerHTML = `
                        <svg viewBox="0 0 24 24" width="16" height="16" aria-hidden="true" focusable="false">
                            <path d="M18 6L17.1991 18.0129C17.129 19.065 17.0939 19.5911 16.8667 19.99C16.6666 20.3412 16.3648 20.6235 16.0011 20.7998C15.588 21 15.0607 21 14.0062 21H9.99377C8.93927 21 8.41202 21 7.99889 20.7998C7.63517 20.6235 7.33339 20.3412 7.13332 19.99C6.90607 19.5911 6.871 19.065 6.80086 18.0129L6 6M4 6H20M16 6L15.7294 5.18807C15.4671 4.40125 15.3359 4.00784 15.0927 3.71698C14.8779 3.46013 14.6021 3.26132 14.2905 3.13878C13.9376 3 13.523 3 12.6936 3H11.3064C10.477 3 10.0624 3 9.70951 3.13878C9.39792 3.26132 9.12208 3.46013 8.90729 3.71698C8.66405 4.00784 8.53292 4.40125 8.27064 5.18807L8 6M14 10V17M10 10V17" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path>
                        </svg>
                    `;
                    removeButton.addEventListener("mousedown", function(evt)
                    {
                        evt.preventDefault();
                    });
                    removeButton.addEventListener("click", function(evt)
                    {
                        evt.preventDefault();
                        evt.stopPropagation();
                        delete item[attribute.name];
                        row.remove();
                    });
                    valueCell.appendChild(removeButton);
                    if(valueCell.childNodes.length == 1)
                        valueCell.insertBefore(document.createTextNode("\u200B"), removeButton);
                }
            }
        }
        else
        {
            inspector.addAttributeValue("name", item.name);
            if(isTopGroup)
                inspector.addAttributeValue("tick_duration", item.tick_duration !== undefined ? item.tick_duration : controller.tick_duration);
            else
                inspector.addAttributeValue("color", item.color || "black");
            for(const p of rowTemplate)
                inspector.addAttributeValue(p.name, item[p.name]);
        }
        for(const p of readOnlyRows)
            inspector.addAttributeValue(p.key, p.value);

        if(main.edit_mode)
        {
            const actionRow = current_t_body.insertRow(-1);
            const actionCellLeft = actionRow.insertCell(0);
            const actionCellRight = actionRow.insertCell(1);
            actionCellLeft.innerHTML = "";
            actionCellRight.className = "inspector-action-cell";
            actionCellRight.style.textAlign = "right";
            actionCellRight.innerHTML = '<button type="button" class="inspector-plus-button" aria-label="Add">+</button>';

            const plusButton = actionCellRight.querySelector(".inspector-plus-button");
            if(plusButton)
            {
                const openNewAttributeRow = function()
                {
                    const row = document.createElement("tr");
                    const labelCell = document.createElement("td");
                    const valueCell = document.createElement("td");
                    labelCell.className = "textedit inspector-attr-cell inspector-attr-cell-label";
                    valueCell.className = "textedit inspector-attr-cell";
                    labelCell.contentEditable = true;
                    valueCell.contentEditable = true;
                    labelCell.setAttribute("data-placeholder", "label");
                    valueCell.setAttribute("data-placeholder", "value");
                    row.appendChild(labelCell);
                    row.appendChild(valueCell);
                    current_t_body.insertBefore(row, actionRow);

                    const commitAttribute = function()
                    {
                        const attrName = labelCell.innerText.trim();
                        if(attrName === "")
                            return;
                        const rawValue = valueCell.innerText.trim();
                        let parsedValue = rawValue;
                        if(rawValue === "true")
                            parsedValue = true;
                        else if(rawValue === "false")
                            parsedValue = false;
                        else if(rawValue !== "" && !isNaN(rawValue))
                            parsedValue = Number(rawValue);
                        item[attrName] = parsedValue;
                        inspector.showGroupBackground(selector.selected_background);
                    };

                    const blurOnEnter = function(evt)
                    {
                        if(evt.keyCode == 13)
                        {
                            evt.preventDefault();
                            evt.target.blur();
                        }
                    };

                    let movingFromLabelToValue = false;
                    const moveFocusToValue = function(evt)
                    {
                        if(evt.key == "Enter" || evt.key == "Tab")
                        {
                            evt.preventDefault();
                            movingFromLabelToValue = true;
                            setTimeout(function()
                            {
                                valueCell.focus();
                            }, 0);
                        }
                    };

                    labelCell.addEventListener("keydown", moveFocusToValue);
                    valueCell.addEventListener("keypress", blurOnEnter);
                    labelCell.addEventListener("blur", function()
                    {
                        labelCell.contentEditable = false;
                        labelCell.setAttribute("contenteditable", "false");
                        labelCell.classList.remove("textedit", "inspector-attr-cell");
                        labelCell.classList.add("inspector-attr-cell-locked");
                        labelCell.removeAttribute("data-placeholder");
                        labelCell.tabIndex = -1;
                        if(movingFromLabelToValue)
                        {
                            movingFromLabelToValue = false;
                            return;
                        }
                        commitAttribute();
                    });
                    labelCell.addEventListener("mousedown", function(evt)
                    {
                        if(labelCell.classList.contains("inspector-attr-cell-locked"))
                            evt.preventDefault();
                    });
                    valueCell.addEventListener("blur", commitAttribute);
                    labelCell.focus();
                };

                const requestNewAttributeRow = function()
                {
                    inspector.pendingAddAttributeRow = true;
                    const active = document.activeElement;
                    if(active && active.closest && active.closest("#component_inspector") && typeof active.blur == "function")
                        active.blur();
                    setTimeout(function()
                    {
                        if(inspector.pendingAddAttributeRow)
                            inspector.showGroupBackground(selector.selected_background);
                    }, 0);
                };

                plusButton.addEventListener("mousedown", function(evt)
                {
                    evt.preventDefault();
                    evt.stopPropagation();
                    requestNewAttributeRow();
                });

                plusButton.addEventListener("click", function(evt)
                {
                    evt.preventDefault();
                    evt.stopPropagation();
                });

                if(inspector.pendingAddAttributeRow)
                {
                    inspector.pendingAddAttributeRow = false;
                    openNewAttributeRow();
                }
            }
        }
        else
        {
            inspector.addAttributeValue("name", item.name);
            if(!isTopGroup)
                inspector.addAttributeValue("color", item.color || "black");
            inspector.addRuntimeLogLevelControl(item);
        }

        for(const p of countRows)
            inspector.addAttributeValue(p.key, p.value);

    },

    showSingleSelection(c) 
    {
        const item = network.dict[c];
        inspector.hideSubviews();
        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';
    
        if (!item) 
        {
            inspector.addHeader("Internal Error");
            return;
        }
    
        if (!item._tag) {
            inspector.addHeader("Unknown Component");
            inspector.addAttributeValue("name", item.name);
            inspector.addDataRows(item, inspector.createTemplate(item));
            return;
        }
    
        const editMode = main.edit_mode;
        const getParameterRows = function(component)
        {
            if(!component || component.parameters == null)
                return [];
            if(Array.isArray(component.parameters))
                return component.parameters;
            if(typeof component.parameters == "object")
                return Object.values(component.parameters);
            return [];
        };
        const commonDataRow = [
            {'name': 'name', 'control': 'textedit', 'type': 'source'}
        ];
        const nameOnlyDataRow = [
            {'name': 'name', 'control': 'textedit', 'type': 'source'}
        ];
        switch (item._tag) {
            case "module":
                inspector.addHeader("MODULE");
                if (editMode) {
                    inspector.addDataRows(item, nameOnlyDataRow, inspector);
                    const moduleClassMenu = inspector.addMenu("class", item.class, network.classes);
                    const applyModuleClass = function () {
                        network.changeModuleClass(c, this.value);
                        selector.selectItems([c], null, false, false, true);
                    };
                    moduleClassMenu.addEventListener('input', applyModuleClass);
                    moduleClassMenu.addEventListener('change', applyModuleClass);
                    const template = getParameterRows(item);
                    inspector.addDataRows(item, template, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                    inspector.addAttributeValue("class", item.class);
                    inspector.addRuntimeLogLevelControl(item);
                }

                break;
            
            case "group":
                inspector.addHeader("GROUP");
                if (editMode) {
                    if(!item.color)
                        item.color = "black";
                    inspector.addDataRows(item, commonDataRow, inspector);
                    inspector.addDataRows(item, [{
                        'name': 'color',
                        'control': 'ui_color',
                        'type': 'source'
                    }], inspector);
                    const logLevelMenu = inspector.addMenu("log_level", logLevelOptions[item.log_level ?? 0], logLevelOptions);
                    const applyGroupLogLevel = function()
                    {
                        item.log_level = logLevelOptions.indexOf(this.value);
                        selector.setLogLevel(item.log_level);
                    };
                    logLevelMenu.addEventListener("input", applyGroupLogLevel);
                    logLevelMenu.addEventListener("change", applyGroupLogLevel);
                } else {
                    inspector.addAttributeValue("name", item.name);
                    inspector.addAttributeValue("color", item.color || "black");
                    inspector.addRuntimeLogLevelControl(item);
                }
                break;
            
            case "input":
                inspector.addHeader("INPUT");
                if (editMode) {
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "output":
                inspector.addHeader("OUTPUT");
                if (editMode) {
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "widget":
                const widgetContainer = document.getElementById(`${selector.selected_background}.${item.name}`);
                inspector.addHeader("WIDGET");
                if (editMode) {
                    const widgetClassMenu = inspector.addMenu("class", item.class, widget_classes);
                    const applyWidgetClass = function () {
                        network.changeWidgetClass(c, this.value);
                        selector.selectItems([c], null, false, false, true);
                    };
                    widgetClassMenu.addEventListener('input', applyWidgetClass);
                    widgetClassMenu.addEventListener('change', applyWidgetClass);
                    const template = widgetContainer.widget.parameter_template;
                    inspector.addDataRows(item, template, widgetContainer.widget);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
    
            default:
                inspector.addHeader("Unknown Tag");
                inspector.addAttributeValue("name", item.name);
                break;
        }
    },

    showConnection(connection)
    {
        const item = network.dict[connection];
        item.source_range = getStringAfterBracket(item.source || "");
        item.target_range = getStringAfterBracket(item.target || "");

        inspector.hideSubviews();
        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';

        inspector.addHeader("CONNECTION");
        inspector.addAttributeValue("source", item.source);
        inspector.addAttributeValue("target", item.target);

            if(main.edit_mode)
            {
                if(!item.color)
                    item.color = "black";
                if(!item.line_type)
                    item.line_type = "auto_route";
                inspector.addDataRows(item, 
                [
                    {'name':'source_range', 'label':'source range', 'control':'range-editor', 'type':'range'},
                    {'name':'target_range', 'label':'target range', 'control':'range-editor', 'type':'range'},
                    {'name':'delay', 'control':'textedit', 'type':'delay'},
                    {'name':'label', 'control':'textedit', 'type':'source'}     
                ], this);
                const connectionRows = [];
                connectionRows.push({
                    'name':'color',
                    'control':'ui_color',
                    'type':'source'
                });
                connectionRows.push({
                    'name':'line_type',
                    'control':'menu',
                    'type':'source',
                    'options':'line,orthogonal,orthagonal rounded,spline,auto_route'
                });
                inspector.addDataRows(item, connectionRows, this);
            }
            else
            {
                inspector.addAttributeValue("delay_range", item.delay_range);
                inspector.addAttributeValue("label", item.label);
            }
    },

    showMultipleSelection(n)
    {
        inspector.hideSubviews();
        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';
        inspector.addHeader("Multiple");
        inspector.addAttributeValue("selected", n); 
    },

    showInspectorForSelection(reveal_inspector=false)
    {
        inspector.revealSelectionPanel(reveal_inspector);
        inspector.updateSelectionPanelWidth();

        if(selector.selected_connection)
            inspector.showConnection(selector.selected_connection)
        else if(selector.selected_foreground.length == 0)
            inspector.showGroupBackground(selector.selected_background);
        else if(selector.selected_foreground.length > 1)
            inspector.showMultipleSelection(selector.selected_foreground.length);
        else
            inspector.showSingleSelection(selector.selected_foreground[0]);
    }
}
