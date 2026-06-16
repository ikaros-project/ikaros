const main = 
{
    view: null,
    grid: null,
    connections: "",
    component_rectangles: [],
    output_debug_lines: [],
    input_debug_lines: [],
    horizontal_routing_lines: [],
    vertical_routing_lines: [],
    routing_grid_points: [],
    grid_spacing: 24,
    grid_active: false,
    edit_mode: false,
    map: {},
    reconnect_overlay: null,
    component_color_property: "color",
    component_clipboard: null,

    new_position_x: 100,
    new_position_y: 100,

    // Initialization and top-level UI setup.
    init()
    {
        main.main = document.querySelector("#main");
        main.workspace = document.querySelector("#main_workspace");
        main.active_pane = main.getPaneElements(document.querySelector(".main_pane.active"));
        main.applyActivePaneReferences();
        main.createWorkspaceConnectionLayer();
        main.observeWorkspaceLayout();
        main.createContextMenu();
        main.createComponentColorMenu();
        main.createWidgetMenu();
        main.createReconnectOverlay();
        main.drawGrid();
        window.addEventListener("resize", () => main.refreshAllPaneGeometry(), false);
        document.addEventListener("click", main.handleWorkspaceConnectionClick, true);
        document.addEventListener("dblclick", main.handleWorkspaceConnectionClick, true);
        main.installPaneEvents(main.active_pane.root);
    },

    getPaneElements(pane)
    {
        main.ensurePaneId(pane);
        return {
            root: pane,
            breadcrumb_strip: pane ? pane.querySelector(".main_breadcrumb_strip") : null,
            surface: pane ? pane.querySelector(".main_pane_surface") : null,
            grid_canvas: pane ? pane.querySelector(".main_pane_grid_canvas") : null,
            connections_layer: pane ? pane.querySelector(".main_pane_connections_layer") : null,
            component_layer: pane ? pane.querySelector(".main_pane_component_layer") : null,
            auto_routing_toggle_button: pane ? pane.querySelector(".auto_routing_toggle_button") : null
        };
    },

    ensurePaneId(pane)
    {
        if(!pane)
            return "";
        if(!main.pane_sequence)
            main.pane_sequence = 1;
        if(!pane.id)
            pane.id = `main_pane_${++main.pane_sequence}`;
        return pane.id;
    },

    getPaneDomId(logicalId, pane=main.active_pane)
    {
        const root = pane && pane.root ? pane.root : (pane && pane.classList ? pane : null);
        const paneId = main.ensurePaneId(root);
        return `${paneId}__${logicalId}`;
    },

    getPaneElement(logicalId, pane=main.active_pane)
    {
        if(!logicalId)
            return null;
        const root = pane && pane.root ? pane.root : (pane && pane.classList ? pane : null);
        if(root && typeof CSS !== "undefined" && typeof CSS.escape === "function")
            return root.querySelector(`[data-logical-id="${CSS.escape(logicalId)}"]`);
        return document.getElementById(logicalId);
    },

    getElementLogicalId(element)
    {
        return element && element.dataset && element.dataset.logicalId ? element.dataset.logicalId : (element ? element.id : "");
    },

    getAllPanes(root=main.workspace || document)
    {
        return root && root.querySelectorAll ? Array.from(root.querySelectorAll(".main_pane")) : [];
    },

    getPaneBackground(pane)
    {
        return pane && pane.dataset ? pane.dataset.background || "" : "";
    },

    getCurrentPaneRoot()
    {
        return main.active_pane && main.active_pane.root ? main.active_pane.root : null;
    },

    isActivePane(pane)
    {
        return !!(pane && pane.classList && pane.classList.contains("active"));
    },

    clearSelectionsOutsidePane(activePane=null)
    {
        const selectedPane = activePane || main.getCurrentPaneRoot();
        for(const pane of main.getAllPanes())
        {
            if(pane === selectedPane)
                continue;
            for(const element of pane.querySelectorAll(".gi.selected"))
                element.classList.remove("selected");
        }
    },

    setPaneBackground(pane, background)
    {
        if(pane && pane.dataset && background)
        {
            pane.dataset.background = background;
            const side = main.getMirrorSideForBackground(background);
            if(side)
            {
                pane.dataset.mirrorSide = side;
                pane.classList.toggle("mirror_right", side === "right");
            }
            else
            {
                delete pane.dataset.mirrorSide;
                pane.classList.remove("mirror_right");
            }
        }
    },

    getPanesForBackground(background, excludePane=null)
    {
        if(!background)
            return [];
        return main.getAllPanes().filter((pane) => pane !== excludePane && main.getPaneBackground(pane) === background);
    },

    getPanesShowingBackground(background)
    {
        return main.getPanesForBackground(background);
    },

    withPane(pane, callback)
    {
        if(!pane || typeof callback !== "function")
            return null;

        const previousPane = main.active_pane;
        main.active_pane = main.getPaneElements(pane);
        main.applyActivePaneReferences();

        try
        {
            return callback(main.active_pane);
        }
        finally
        {
            main.active_pane = previousPane;
            main.applyActivePaneReferences();
        }
    },

    getFrameWidget(frame)
    {
        if(!frame)
            return null;
        if(typeof frame.querySelector === "function")
            return frame.querySelector(":scope > .widget");
        return null;
    },

    updatePaneWidgetFrames(pane=null)
    {
        const root = pane || document;
        const frames = root.querySelectorAll ? root.querySelectorAll(".frame") : [];
        frames.forEach((frame) =>
        {
            const widgetElement = main.getFrameWidget(frame);
            if(widgetElement && typeof widgetElement.updateFrame === "function")
                widgetElement.updateFrame();
        });
    },

    applyActivePaneReferences()
    {
        if(!main.active_pane)
            return;
        main.view = main.active_pane.component_layer;
        main.grid = main.active_pane.surface;
        main.grid_canvas = main.active_pane.grid_canvas;
        main.auto_routing_toggle_button = main.active_pane.auto_routing_toggle_button;
    },

    installPaneEvents(pane)
    {
        const elements = main.getPaneElements(pane);
        if(elements.root && elements.root.dataset.mainPaneEventsInstalled !== "true")
        {
            elements.root.addEventListener("mousedown", main.activatePaneFromEvent, true);
            elements.root.dataset.mainPaneEventsInstalled = "true";
        }
        if(!elements.component_layer || elements.component_layer.dataset.mainEventsInstalled === "true")
            return;
        elements.component_layer.addEventListener("mousedown", main.startBackgroundSelection, false);
        elements.component_layer.addEventListener("contextmenu", main.showBackgroundContextMenu, false);
        elements.component_layer.dataset.mainEventsInstalled = "true";
    },

    activatePaneFromEvent(evt)
    {
        const pane = evt.currentTarget;
        if(!pane || pane.classList.contains("active"))
            return;
        main.activatePane(pane, false);
    },

    createPane()
    {
        const pane = document.createElement("section");
        main.ensurePaneId(pane);
        pane.className = "main_pane";
        pane.innerHTML = `
            <div class="main_breadcrumb_strip"></div>
            <div class="main_pane_surface">
                <div class="main_grid_controls">
                    <button onclick="main.decreaseGrid()" style="border-radius:15px 0 0 15px; margin-left: 0">-</button>
                    <button onclick="main.increaseGrid()">+</button>
                    <button onclick="main.arrangeComponents()">arrange</button>
                    <button class="auto_routing_toggle_button" onclick="main.toggleBackgroundAutoRouting()">auto route</button>
                    <button class="command_info_button" onclick="main.showCommandInfo()" style="border-radius:0 15px 15px 0;"><span class="info-glyph">i</span></button>
                </div>
                <div class="component_create_controls">
                    <button onclick="main.newModule()" style="border-radius:15px 0 0 15px; margin-left: 0">Module</button>
                    <button onclick="main.newGroup()">Group</button>
                    <button onclick="main.newInput()">Input</button>
                    <button onclick="main.newOutput()">Output</button>
                    <button onclick="main.newWidget()" style="border-radius:0 15px 15px 0;">Widget</button>
                </div>
                <canvas class="main_pane_grid_canvas" width="3000" height="3000"></canvas>
            </div>
            <div class="main_pane_connections_layer"></div>
            <div class="main_pane_component_layer"></div>
        `;
        main.installPaneEvents(pane);
        return pane;
    },

    resetSplitLayout()
    {
        if(!main.workspace)
            return;

        const currentPane = main.active_pane && main.active_pane.root ? main.active_pane.root : document.querySelector(".main_pane");
        const pane = currentPane || main.createPane();
        document.querySelectorAll(".main_pane.active").forEach((p) => p.classList.remove("active"));
        pane.classList.add("active");

        if(pane.parentElement !== main.workspace || main.workspace.children.length !== 1)
        {
            main.workspace.innerHTML = "";
            main.workspace.className = "split_node";
            main.workspace.appendChild(pane);
        }

        main.active_pane = main.getPaneElements(pane);
        main.applyActivePaneReferences();
        main.installPaneEvents(pane);
        main.drawGrid();
        main.savePaneLayout();
    },

    getPaneLayoutStorageKey()
    {
        const networkName = network && network.network && network.network.name ? network.network.name : "default";
        return `ikaros_pane_layout:${networkName}`;
    },

    getPaneLayoutMetadataValue()
    {
        return network && network.network ? network.network._pane_layout : null;
    },

    setPaneLayoutMetadataValue(layout)
    {
        if(!network || !network.network)
            return;
        // UI metadata saved with the network file; pane layout changes do not taint the model.
        if(layout)
            network.network._pane_layout = JSON.stringify(layout);
        else
            delete network.network._pane_layout;
    },

    isValidPaneLayout(layout)
    {
        return !!(layout && layout.version === 1 && layout.tree);
    },

    parsePaneLayoutValue(value)
    {
        if(!value)
            return null;
        if(typeof value === "object")
            return value;
        if(typeof value !== "string")
            return null;
        try
        {
            return JSON.parse(value);
        }
        catch(err)
        {
            console.log("pane layout parse failed", err);
            return null;
        }
    },

    getStoredPaneLayout()
    {
        const fileLayout = main.parsePaneLayoutValue(main.getPaneLayoutMetadataValue());
        if(main.isValidPaneLayout(fileLayout))
            return fileLayout;

        try
        {
            const raw = localStorage.getItem(main.getPaneLayoutStorageKey());
            const storedLayout = main.parsePaneLayoutValue(raw);
            return main.isValidPaneLayout(storedLayout) ? storedLayout : null;
        }
        catch(err)
        {
            console.log("pane layout storage read failed", err);
            return null;
        }
    },

    getPaneLayoutBackground(background, fallback=true)
    {
        if(background && network && network.dict && network.dict[background])
            return background;
        if(!fallback)
            return "";
        if(selector && selector.selected_background && network && network.dict && network.dict[selector.selected_background])
            return selector.selected_background;
        return network && network.network && network.network.name ? network.network.name : "";
    },

    clampPaneLayoutRatio(ratio)
    {
        const value = Number(ratio);
        if(!Number.isFinite(value))
            return 50;
        return Math.max(5, Math.min(95, value));
    },

    serializePaneLayoutNode(node)
    {
        if(!node)
            return null;
        if(node.classList && node.classList.contains("main_pane"))
        {
            return {
                type: "pane",
                background: main.getPaneBackground(node),
                active: node.classList.contains("active")
            };
        }
        if(node.classList && node.classList.contains("split_node"))
        {
            const children = Array.from(node.children).filter((child) => !child.classList.contains("split_resize_handle"));
            if(children.length === 1)
                return main.serializePaneLayoutNode(children[0]);
            if(children.length !== 2)
                return null;
            const direction = node.classList.contains("split_horizontal") ? "horizontal" : "vertical";
            const ratio = parseFloat(node.style.getPropertyValue("--split-ratio")) || 50;
            return {
                type: "split",
                direction,
                splitType: main.getSplitType(node),
                ratio,
                mirrorLocked: main.isMirrorLocked(node),
                children: children.map((child) => main.serializePaneLayoutNode(child))
            };
        }
        return null;
    },

    serializePaneLayout()
    {
        if(!main.workspace)
            return null;
        const root = Array.from(main.workspace.children).find((child) => child.classList && (child.classList.contains("main_pane") || child.classList.contains("split_node")));
        const tree = main.serializePaneLayoutNode(root);
        if(!tree)
            return null;
        return {
            version: 1,
            network: network && network.network ? network.network.name : "",
            tree
        };
    },

    writePaneLayout(layout)
    {
        if(!layout)
            return;
        main.setPaneLayoutMetadataValue(layout);
        try
        {
            localStorage.setItem(main.getPaneLayoutStorageKey(), JSON.stringify(layout));
        }
        catch(err)
        {
            console.log("pane layout localStorage save failed", err);
        }
    },

    savePaneLayout()
    {
        if(main.restoring_pane_layout)
            return;
        try
        {
            const layout = main.serializePaneLayout();
            if(layout)
                main.writePaneLayout(layout);
        }
        catch(err)
        {
            console.log("pane layout save failed", err);
        }
    },

    clearSavedPaneLayout()
    {
        try
        {
            main.setPaneLayoutMetadataValue(null);
            localStorage.removeItem(main.getPaneLayoutStorageKey());
        }
        catch(err)
        {
            console.log("pane layout clear failed", err);
        }
    },

    resetSavedPaneLayout()
    {
        main.clearSavedPaneLayout();
        main.resetSplitLayout();
        if(selector && selector.selected_background)
            selector.selectItems([], selector.selected_background, false, false, true);
    },

    buildPaneLayoutNode(layoutNode)
    {
        if(!layoutNode || !network || !network.dict)
            return null;

        if(layoutNode.type === "pane")
        {
            const pane = main.createPane();
            const background = main.getPaneLayoutBackground(layoutNode.background, false);
            if(!background)
            {
                main.pane_layout_restore_pruned = true;
                return null;
            }
            main.setPaneBackground(pane, background);
            main.renderIntoPane(pane, background, []);
            if(layoutNode.active)
                pane.classList.add("active");
            return pane;
        }

        if(layoutNode.type === "split" && Array.isArray(layoutNode.children) && layoutNode.children.length === 2)
        {
            const firstChild = main.buildPaneLayoutNode(layoutNode.children[0]);
            const secondChild = main.buildPaneLayoutNode(layoutNode.children[1]);
            if(!firstChild || !secondChild)
            {
                main.pane_layout_restore_pruned = true;
                return firstChild || secondChild;
            }

            const direction = layoutNode.direction === "horizontal" ? "horizontal" : "vertical";
            const splitNode = document.createElement("div");
            splitNode.className = `split_node split_${direction}`;
            if(layoutNode.splitType === "mirror")
                splitNode.classList.add("split_mirror");
            splitNode.dataset.splitType = layoutNode.splitType || "split";
            splitNode.appendChild(firstChild);
            splitNode.appendChild(secondChild);
            splitNode.appendChild(main.createSplitHandle(direction));
            if(layoutNode.splitType === "mirror")
                main.setMirrorLocked(splitNode, layoutNode.mirrorLocked !== false);
            main.setSplitRatio(splitNode, direction, main.clampPaneLayoutRatio(layoutNode.ratio));
            return splitNode;
        }

        return null;
    },

    restorePaneLayout()
    {
        if(!main.workspace || !network || !network.dict)
            return false;

        const layout = main.getStoredPaneLayout();

        if(!main.isValidPaneLayout(layout))
            return false;

        main.restoring_pane_layout = true;
        main.pane_layout_restore_pruned = false;
        try
        {
            const root = main.buildPaneLayoutNode(layout.tree);
            if(!root)
                return false;

            main.workspace.innerHTML = "";
            main.workspace.className = "split_node";
            main.workspace.appendChild(root);

            let activePane = main.workspace.querySelector(".main_pane.active") || main.workspace.querySelector(".main_pane");
            document.querySelectorAll(".main_pane.active").forEach((pane) =>
            {
                if(pane !== activePane)
                    pane.classList.remove("active");
            });
            if(activePane)
                activePane.classList.add("active");
            main.getAllPanes().forEach((pane) => main.installPaneEvents(pane));
            main.getAllPanes().forEach((pane) =>
            {
                const background = main.getPaneBackground(pane);
                if(background)
                    main.renderIntoPane(pane, background, []);
            });
            if(activePane)
                main.activatePane(activePane, false);
            main.refreshAllPaneGeometry();
            main.updatePaneWidgetFrames();
            if(main.pane_layout_restore_pruned)
                main.writePaneLayout(main.serializePaneLayout());
            return !!activePane;
        }
        finally
        {
            main.restoring_pane_layout = false;
            main.pane_layout_restore_pruned = false;
        }
    },

    activatePane(pane, render=true)
    {
        if(!pane)
            return;
        const paneChanged = !(main.active_pane && main.active_pane.root === pane);
        document.querySelectorAll(".main_pane.active").forEach((p) => p.classList.remove("active"));
        pane.classList.add("active");
        main.active_pane = main.getPaneElements(pane);
        main.applyActivePaneReferences();
        main.installPaneEvents(pane);
        main.drawGrid();
        if(nav && typeof nav.setTargetPane === "function")
            nav.setTargetPane(pane);
        const paneBackground = main.getPaneBackground(pane);
        if(selector && paneBackground)
        {
            if(paneBackground !== selector.selected_background || paneChanged)
            {
                selector.selected_background = paneBackground;
                selector.selected_foreground = [];
                selector.clearConnectionSelection();
            }
            setCookie("selected_background", selector.selected_background);
            nav.selectItem(selector.selected_background, pane);
        }
        if(selector && selector.selected_background)
        {
            breadcrumbs.selectItem(selector.selected_background);
            if(render)
                main.selectItem(selector.selected_foreground, selector.selected_background);
            else
            {
                main.updateComponentStates();
                main.updateAutoRoutingButtonState();
            }
            if(inspector && typeof inspector.showInspectorForSelection === "function")
                inspector.showInspectorForSelection(false);
        }
        main.savePaneLayout();
    },

    getPaneFromSource(source)
    {
        if(source && typeof source.closest === "function")
            return source.closest(".main_pane");
        return main.active_pane ? main.active_pane.root : null;
    },

    splitHorizontal(source=null)
    {
        main.splitPane("horizontal", main.getPaneFromSource(source));
    },

    splitVertical(source=null)
    {
        main.splitPane("vertical", main.getPaneFromSource(source));
    },

    splitMirror(source=null)
    {
        main.splitPane("vertical", main.getPaneFromSource(source), "mirror");
    },

    getMirrorSideForBackground(background)
    {
        const rootPath = main.getMirrorRootPathForBackground(background);
        const group = rootPath && network && network.dict ? network.dict[rootPath] : null;
        return group && (group._mirror_side === "left" || group._mirror_side === "right") ? group._mirror_side : "";
    },

    getMirrorRootPathForBackground(background)
    {
        let path = background || "";
        while(path && network && network.dict)
        {
            const group = network.dict[path];
            if(group && (group._mirror_side === "left" || group._mirror_side === "right"))
                return path;
            const parent = parentPath(path);
            if(!parent || parent === path)
                break;
            path = parent;
        }
        return "";
    },

    getPaneMirrorSide(pane=null)
    {
        const root = pane || (main.active_pane ? main.active_pane.root : null);
        return root && root.dataset ? root.dataset.mirrorSide || "" : "";
    },

    isMirrorBackground(background)
    {
        return !!main.getMirrorSideForBackground(background);
    },

    isRightMirrorBackground(background)
    {
        return main.getMirrorSideForBackground(background) === "right";
    },

    getMirrorPaneWidth()
    {
        const root = main.active_pane && main.active_pane.root ? main.active_pane.root : null;
        return root ? root.getBoundingClientRect().width : 0;
    },

    modelXToPaneX(x, background=null)
    {
        const side = main.getMirrorSideForBackground(background || (selector ? selector.selected_background : ""));
        const value = Number(x) || 0;
        if(side === "left")
            return main.getMirrorPaneWidth() + value;
        return value;
    },

    getPositionedXStyle(x, background=null)
    {
        const resolvedBackground = background || (selector ? selector.selected_background : "");
        const side = main.getMirrorSideForBackground(resolvedBackground);
        const value = main.modelXToPaneX(x, resolvedBackground);
        if(side === "right")
            return `right:calc(100% - ${value}px);left:auto;`;
        return `left:${value}px;`;
    },

    paneXToModelX(x, background=null)
    {
        const side = main.getMirrorSideForBackground(background || (selector ? selector.selected_background : ""));
        const value = Number(x) || 0;
        if(side === "left")
            return Math.min(0, value - main.getMirrorPaneWidth());
        if(side === "right")
            return Math.max(0, value);
        return Math.max(0, value);
    },

    normalizeNewComponentX(x)
    {
        const side = main.getMirrorSideForBackground(selector ? selector.selected_background : "");
        const value = Number(x) || 0;
        if(side === "left")
            return value <= 0 ? value : -value;
        if(side === "right")
            return Math.max(0, value);
        return value;
    },

    getUniqueChildName(parentPath, baseName)
    {
        if(!parentPath || !baseName)
            return baseName;
        let name = baseName;
        let index = 1;
        while(network && network.dict && network.dict[`${parentPath}.${name}`])
            name = `${baseName}${++index}`;
        return name;
    },

    ensureMirrorChildGroup(parentPath, baseName, side)
    {
        const parent = parentPath && network && network.dict ? network.dict[parentPath] : null;
        if(!parent)
            return "";

        const requestedPath = `${parentPath}.${baseName}`;
        const existing = network.dict[requestedPath];
        if(existing && existing._tag === "group")
        {
            existing._mirror_side = side;
            return requestedPath;
        }

        if(!parent.groups)
            parent.groups = [];
        const name = existing ? main.getUniqueChildName(parentPath, baseName) : baseName;
        const group = {
            name,
            color: "",
            _tag: "group",
            _x: side === "left" ? -140 : 140,
            _y: 100,
            _mirror_side: side,
            inputs: [],
            outputs: [],
            parameters: [],
            modules: [],
            groups: [],
            widgets: [],
            connections: []
        };
        parent.groups.push(group);
        return `${parentPath}.${name}`;
    },

    ensureMirrorSplitGroups(parentPath)
    {
        if(!parentPath)
            return null;
        const leftPath = main.ensureMirrorChildGroup(parentPath, "Left", "left");
        const rightPath = main.ensureMirrorChildGroup(parentPath, "Right", "right");
        if(!leftPath || !rightPath)
            return null;
        network.rebuildDict();
        nav.populate();
        return {leftPath, rightPath};
    },

    getMirrorCounterpartBackground(background)
    {
        const mirrorRootPath = main.getMirrorRootPathForBackground(background);
        const group = mirrorRootPath && network && network.dict ? network.dict[mirrorRootPath] : null;
        const side = group && group._mirror_side;
        if(side !== "left" && side !== "right")
            return "";
        const parent = parentPath(mirrorRootPath);
        const parentGroup = parent && network.dict ? network.dict[parent] : null;
        const counterpartSide = side === "left" ? "right" : "left";
        const child = (parentGroup && parentGroup.groups || []).find((candidate) => candidate && candidate._mirror_side === counterpartSide);
        if(!child)
            return "";
        const counterpartRootPath = `${parent}.${child.name}`;
        if(background === mirrorRootPath)
            return counterpartRootPath;
        const relativePath = removeStringFromStart(background, `${mirrorRootPath}.`);
        const counterpartPath = `${counterpartRootPath}.${relativePath}`;
        return network.dict && network.dict[counterpartPath] ? counterpartPath : "";
    },

    mirrorComponentX(value, targetSide)
    {
        const x = Number(value) || 0;
        return targetSide === "left" ? -Math.abs(x) : Math.abs(x);
    },

    mirrorComponentForSide(component, targetSide)
    {
        const clone = deepCopy(component);
        if(clone && clone._x !== undefined)
            clone._x = main.mirrorComponentX(clone._x, targetSide);
        if(clone && clone._tag === "group")
        {
            clone.inputs = (clone.inputs || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            clone.outputs = (clone.outputs || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            clone.modules = (clone.modules || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            clone.widgets = (clone.widgets || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            clone.groups = (clone.groups || []).map((item) => main.mirrorComponentForSide(item, targetSide));
        }
        return clone;
    },

    mirrorConnectionEndpoint(endpoint, sourcePath, targetPath)
    {
        if(typeof endpoint !== "string" || !sourcePath || !targetPath)
            return endpoint;
        const base = getStringUpToBracket(endpoint);
        const suffix = endpoint.substring(base.length);
        if(base === sourcePath)
            return targetPath + suffix;
        if(base.startsWith(sourcePath + "."))
            return base.substring(sourcePath.length + 1) + suffix;
        if(base.startsWith(targetPath + "."))
            return base.substring(targetPath.length + 1) + suffix;
        return endpoint;
    },

    mirrorConnectionForBackground(connection, sourcePath, targetPath)
    {
        const clone = deepCopy(connection);
        if(!clone)
            return clone;
        clone.source = main.mirrorConnectionEndpoint(clone.source, sourcePath, targetPath);
        clone.target = main.mirrorConnectionEndpoint(clone.target, sourcePath, targetPath);
        return clone;
    },

    getMirroredEndpointBaseForLockedMirror(endpointBase)
    {
        const mirrorRootPath = main.getMirrorRootPathForBackground(endpointBase);
        if(!mirrorRootPath || !main.isLockedMirrorBackground(mirrorRootPath))
            return "";
        const counterpartRootPath = main.getMirrorCounterpartBackground(mirrorRootPath);
        if(!counterpartRootPath)
            return "";
        if(endpointBase === mirrorRootPath)
            return counterpartRootPath;
        if(!endpointBase.startsWith(`${mirrorRootPath}.`))
            return "";
        const relativePath = endpointBase.substring(mirrorRootPath.length + 1);
        return `${counterpartRootPath}.${relativePath}`;
    },

    addMirroredConnectionForEndpoints(sourceEndpointId, targetEndpointId, sourceConnection=null)
    {
        if(main.mirror_syncing)
            return "";
        const sourceBase = main.getConnectionEndpointBase(sourceEndpointId);
        const targetBase = main.getConnectionEndpointBase(targetEndpointId);
        const mirroredSourceBase = main.getMirroredEndpointBaseForLockedMirror(sourceBase) || sourceBase;
        const mirroredTargetBase = main.getMirroredEndpointBaseForLockedMirror(targetBase) || targetBase;
        if(mirroredSourceBase === sourceBase && mirroredTargetBase === targetBase)
            return "";
        if(!network || !network.dict)
            return "";

        const mirroredConnectionPath = main.getConnectionOwnerPath(mirroredSourceBase, mirroredTargetBase);
        const mirroredGroup = mirroredConnectionPath && network && network.dict ? network.dict[mirroredConnectionPath] : null;
        if(!mirroredGroup || mirroredGroup._tag !== "group")
            return "";

        const cleanSource = main.getRelativeConnectionEndpoint(mirroredSourceBase, mirroredConnectionPath);
        const cleanTarget = main.getRelativeConnectionEndpoint(mirroredTargetBase, mirroredConnectionPath);
        if(!mirroredGroup.connections)
            mirroredGroup.connections = [];
        const alreadyExists = mirroredGroup.connections.some((connection) =>
            connection && connection.source === cleanSource && connection.target === cleanTarget
        );
        if(alreadyExists)
            return mirroredConnectionPath;

        const mirrorConnection = sourceConnection ? deepCopy(sourceConnection) : {
            _tag: "connection",
            delay: "1",
            color: "black",
            line_type: "auto_route"
        };
        mirrorConnection.source = cleanSource;
        mirrorConnection.target = cleanTarget;
        mirroredGroup.connections.push(mirrorConnection);
        network.rebuildDict();
        return mirroredConnectionPath;
    },

    updateLockedMirrorComponentPosition(sourceFullName)
    {
        if(main.mirror_syncing || !sourceFullName || !network || !network.dict)
            return;
        const sourceComponent = network.dict[sourceFullName];
        if(!sourceComponent)
            return;
        const mirrorRootPath = main.getMirrorRootPathForBackground(sourceFullName);
        if(!mirrorRootPath || !main.isLockedMirrorBackground(mirrorRootPath))
            return;
        const counterpartRootPath = main.getMirrorCounterpartBackground(mirrorRootPath);
        if(!counterpartRootPath || !sourceFullName.startsWith(`${mirrorRootPath}.`))
            return;
        const relativePath = sourceFullName.substring(mirrorRootPath.length + 1);
        const targetFullName = `${counterpartRootPath}.${relativePath}`;
        const targetComponent = network.dict[targetFullName];
        if(!targetComponent)
            return;

        targetComponent._x = main.mirrorComponentX(sourceComponent._x, targetComponent._mirror_side || main.getMirrorSideForBackground(targetFullName));
        targetComponent._y = sourceComponent._y;
        const targetBackground = parentPath(targetFullName);
        for(const pane of main.getPanesForBackground(targetBackground))
        {
            const element = main.getPaneElement(targetFullName, pane);
            main.applyPositionedComponentStyle(element, targetComponent, targetBackground);
        }
    },

    getLockedMirrorCounterpartPath(sourceFullName)
    {
        if(!sourceFullName || !network || !network.dict)
            return "";
        const mirrorRootPath = main.getMirrorRootPathForBackground(sourceFullName);
        if(!mirrorRootPath || !main.isLockedMirrorBackground(mirrorRootPath))
            return "";
        const counterpartRootPath = main.getMirrorCounterpartBackground(mirrorRootPath);
        if(!counterpartRootPath || !sourceFullName.startsWith(`${mirrorRootPath}.`))
            return "";
        const relativePath = sourceFullName.substring(mirrorRootPath.length + 1);
        const targetFullName = `${counterpartRootPath}.${relativePath}`;
        return network.dict[targetFullName] ? targetFullName : "";
    },

    syncLockedMirrorItemProperties(sourceFullName, propertyNames)
    {
        if(main.mirror_syncing || !sourceFullName || !Array.isArray(propertyNames) || propertyNames.length === 0)
            return;
        const source = network && network.dict ? network.dict[sourceFullName] : null;
        const targetFullName = main.getLockedMirrorCounterpartPath(sourceFullName);
        const target = targetFullName ? network.dict[targetFullName] : null;
        if(!source || !target)
            return;

        for(const propertyName of propertyNames)
        {
            if(propertyName === "name" || propertyName === "_tag" || propertyName === "_mirror_side")
                continue;
            target[propertyName] = deepCopy(source[propertyName]);
        }
        const targetBackground = parentPath(targetFullName);
        main.refreshMatchingBackgroundPanes(targetBackground, [], main.active_pane ? main.active_pane.root : null);
        main.refreshAllPaneGeometry();
    },

    getComponentCollectionName(tag)
    {
        return {
            group: "groups",
            module: "modules",
            input: "inputs",
            output: "outputs",
            widget: "widgets"
        }[tag] || "";
    },

    addLockedMirrorComponent(sourceFullName)
    {
        if(main.mirror_syncing || !sourceFullName || !network || !network.dict)
            return;
        const source = network.dict[sourceFullName];
        if(!source)
            return;
        const sourceBackground = parentPath(sourceFullName);
        const targetBackground = main.getMirrorCounterpartBackground(sourceBackground);
        const targetGroup = targetBackground ? network.dict[targetBackground] : null;
        const collectionName = main.getComponentCollectionName(source._tag);
        if(!targetGroup || !collectionName)
            return;
        const targetFullName = `${targetBackground}.${source.name}`;
        if(network.dict[targetFullName])
            return;
        if(!targetGroup[collectionName])
            targetGroup[collectionName] = [];
        targetGroup[collectionName].push(main.mirrorComponentForSide(source, main.getMirrorSideForBackground(targetBackground)));
        network.rebuildDict();
        main.refreshMatchingBackgroundPanes(targetBackground, [], main.active_pane ? main.active_pane.root : null);
        main.refreshAllPaneGeometry();
    },

    deleteLockedMirrorComponent(sourceFullName)
    {
        const targetFullName = main.getLockedMirrorCounterpartPath(sourceFullName);
        const target = targetFullName ? network.dict[targetFullName] : null;
        if(!target)
            return;
        const targetBackground = parentPath(targetFullName);
        const targetGroup = network.dict[targetBackground];
        const collectionName = main.getComponentCollectionName(target._tag);
        if(!targetGroup || !collectionName || !Array.isArray(targetGroup[collectionName]))
            return;
        targetGroup[collectionName] = targetGroup[collectionName].filter((item) => item && item.name !== target.name);
        if(targetGroup.connections)
            main.deleteConnectionsForComponent(target.name, targetGroup);
        network.rebuildDict();
        main.refreshMatchingBackgroundPanes(targetBackground, [], main.active_pane ? main.active_pane.root : null);
        main.refreshAllPaneGeometry();
    },

    renameLockedMirrorComponent(sourceFullName, newName)
    {
        const targetFullName = main.getLockedMirrorCounterpartPath(sourceFullName);
        const target = targetFullName ? network.dict[targetFullName] : null;
        if(!target || !newName)
            return;
        target.name = newName;
        network.rebuildDict();
        const targetBackground = parentPath(targetFullName);
        main.refreshMatchingBackgroundPanes(targetBackground, [], main.active_pane ? main.active_pane.root : null);
        main.refreshAllPaneGeometry();
    },

    syncLockedMirrorCounterpart(background)
    {
        if(main.mirror_syncing || !main.isLockedMirrorBackground(background))
            return;
        const source = network && network.dict ? network.dict[background] : null;
        const targetPath = main.getMirrorCounterpartBackground(background);
        const target = targetPath ? network.dict[targetPath] : null;
        if(!source || !target)
            return;

        const targetName = target.name;
        const targetSide = target._mirror_side;
        const targetX = target._x;
        const targetY = target._y;
        main.mirror_syncing = true;
        try
        {
            target.color = source.color || "";
            target.inputs = (source.inputs || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            target.outputs = (source.outputs || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            target.modules = (source.modules || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            target.widgets = (source.widgets || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            target.groups = (source.groups || []).map((item) => main.mirrorComponentForSide(item, targetSide));
            target.connections = (source.connections || []).map((connection) => main.mirrorConnectionForBackground(connection, background, targetPath));
            target.parameters = deepCopy(source.parameters || []);
            target.name = targetName;
            target._tag = "group";
            target._mirror_side = targetSide;
            target._x = targetX;
            target._y = targetY;
            network.rebuildDict();
            nav.populate();
            main.refreshMatchingBackgroundPanes(targetPath, []);
            main.refreshAllPaneGeometry();
        }
        finally
        {
            main.mirror_syncing = false;
        }
    },

    getSplitType(splitNode)
    {
        return splitNode && splitNode.dataset ? splitNode.dataset.splitType || "split" : "split";
    },

    isMirrorSplitNode(splitNode)
    {
        return main.getSplitType(splitNode) === "mirror";
    },

    isMirrorLocked(splitNode)
    {
        return !splitNode || !splitNode.dataset ? true : splitNode.dataset.mirrorLocked !== "false";
    },

    getMirrorLockSymbol(locked)
    {
        return locked ? "=" : "\u2260";
    },

    setMirrorLocked(splitNode, locked)
    {
        if(!splitNode || !splitNode.dataset)
            return;
        splitNode.dataset.mirrorLocked = locked ? "true" : "false";
        splitNode.classList.toggle("mirror_locked", !!locked);
        splitNode.classList.toggle("mirror_unlocked", !locked);
        const button = splitNode.querySelector(":scope > .split_resize_handle .mirror_lock_toggle");
        if(button)
        {
            button.textContent = main.getMirrorLockSymbol(locked);
            button.title = locked ? "Mirror locked" : "Mirror unlocked";
            button.setAttribute("aria-label", button.title);
        }
    },

    toggleMirrorLock(source)
    {
        const splitNode = source && source.closest ? source.closest(".split_node") : null;
        if(!main.isMirrorSplitNode(splitNode))
            return;
        const locked = !main.isMirrorLocked(splitNode);
        main.setMirrorLocked(splitNode, locked);
        main.savePaneLayout();
    },

    getMirrorSplitForPane(pane=null)
    {
        const root = pane || (main.active_pane ? main.active_pane.root : null);
        const splitNode = root && root.parentElement && root.parentElement.classList && root.parentElement.classList.contains("split_node") ? root.parentElement : null;
        return main.isMirrorSplitNode(splitNode) ? splitNode : null;
    },

    isLockedMirrorBackground(background)
    {
        if(!main.isMirrorBackground(background))
            return false;
        const mirrorRootPath = main.getMirrorRootPathForBackground(background);
        const pane = main.getAllPanes().find((candidate) =>
        {
            const paneBackground = main.getPaneBackground(candidate);
            return paneBackground === background ||
                paneBackground === mirrorRootPath ||
                (paneBackground && paneBackground.startsWith(`${mirrorRootPath}.`));
        });
        const splitNode = main.getMirrorSplitForPane(pane);
        return main.isMirrorLocked(splitNode);
    },

    isMirrorPane(pane)
    {
        const splitNode = pane && pane.closest ? pane.closest(".split_node") : null;
        return main.isMirrorSplitNode(splitNode);
    },

    splitPane(direction, pane=null, splitType="split")
    {
        if(!pane)
            return;
        if(main.isMirrorPane(pane))
            return;
        const wasActive = pane.classList.contains("active");
        const paneBackground = main.getPaneBackground(pane) || (selector ? selector.selected_background : "");
        const mirrorGroups = splitType === "mirror" ? main.ensureMirrorSplitGroups(paneBackground) : null;
        if(splitType === "mirror" && !mirrorGroups)
            return;
        const splitNode = document.createElement("div");
        splitNode.className = `split_node split_${direction}`;
        if(splitType === "mirror")
            splitNode.classList.add("split_mirror");
        splitNode.dataset.splitType = splitType;
        splitNode.style.setProperty("--split-ratio", "50");
        const newPane = main.createPane();
        const handle = main.createSplitHandle(direction);
        pane.replaceWith(splitNode);
        splitNode.appendChild(pane);
        splitNode.appendChild(newPane);
        splitNode.appendChild(handle);
        if(splitType === "mirror")
            main.setMirrorLocked(splitNode, true);
        main.installPaneEvents(pane);
        main.drawGrid(main.getPaneElements(newPane));
        if(splitType === "mirror")
        {
            pane.dataset.mirrorSide = "left";
            newPane.dataset.mirrorSide = "right";
            pane.classList.remove("mirror_right");
            newPane.classList.add("mirror_right");
            main.renderIntoPane(pane, mirrorGroups.leftPath, []);
            main.renderIntoPane(newPane, mirrorGroups.rightPath, []);
        }
        else if(selector && selector.selected_background)
        {
            main.setPaneBackground(newPane, selector.selected_background);
            breadcrumbs.selectItem(selector.selected_background, newPane);
        }
        if(splitType !== "mirror" && selector && selector.selected_background)
            main.renderIntoPane(newPane, selector.selected_background, []);
        if(wasActive)
            main.activatePane(pane, false);
        if(splitType === "mirror")
            selector.selectItems([], mirrorGroups.leftPath, false, false, true);
        if(controller && typeof controller.forceNextUpdate === "function" && controller.run_mode !== "stop")
        {
            requestAnimationFrame(() =>
            {
                main.refreshAllPaneGeometry();
                main.updatePaneWidgetFrames(splitNode);
                const latestData = controller.g_data || (controller.data_package ? controller.data_package.data : null);
                if(latestData)
                    controller.updateImages(latestData);
                controller.forceNextUpdate();
            });
        }
        else
            requestAnimationFrame(() => main.updatePaneWidgetFrames(splitNode));
        main.savePaneLayout();
    },

    createSplitHandle(direction)
    {
        const handle = document.createElement("div");
        handle.className = `split_resize_handle split_resize_handle_${direction}`;
        handle.dataset.direction = direction;
        const button = document.createElement("button");
        button.type = "button";
        button.className = "mirror_lock_toggle";
        button.title = "Mirror locked";
        button.setAttribute("aria-label", "Mirror locked");
        button.textContent = main.getMirrorLockSymbol(true);
        button.addEventListener("mousedown", function(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
        }, false);
        button.addEventListener("click", function(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
            main.toggleMirrorLock(button);
        }, false);
        handle.appendChild(button);
        handle.addEventListener("mousedown", main.startSplitResize, false);
        return handle;
    },

    startSplitResize(evt)
    {
        if(evt.target && evt.target.closest && evt.target.closest(".mirror_lock_toggle"))
            return;
        const handle = evt.currentTarget;
        const splitNode = handle ? handle.parentElement : null;
        if(!splitNode)
            return;

        evt.preventDefault();
        evt.stopPropagation();
        main.split_resize = {
            splitNode,
            direction: handle.dataset.direction
        };
        document.addEventListener("mousemove", main.resizeSplit, true);
        document.addEventListener("mouseup", main.finishSplitResize, true);
    },

    resizeSplit(evt)
    {
        const session = main.split_resize;
        if(!session || !session.splitNode)
            return;

        const rect = session.splitNode.getBoundingClientRect();
        const isVertical = session.direction === "vertical";
        const size = isVertical ? rect.width : rect.height;
        if(size <= 0)
            return;

        const pointer = isVertical ? evt.clientX - rect.left : evt.clientY - rect.top;
        const minPixels = 80;
        const minRatio = Math.min(45, Math.max(5, (minPixels / size) * 100));
        const ratio = Math.max(minRatio, Math.min(100 - minRatio, (pointer / size) * 100));

        main.setSplitRatio(session.splitNode, session.direction, ratio);
        main.refreshAllPaneGeometry();
    },

    finishSplitResize()
    {
        document.removeEventListener("mousemove", main.resizeSplit, true);
        document.removeEventListener("mouseup", main.finishSplitResize, true);
        main.split_resize = null;
        main.refreshAllPaneGeometry();
        main.updatePaneWidgetFrames();
        if(controller && controller.g_data)
            controller.updateImages(controller.g_data);
        main.savePaneLayout();
    },

    setSplitRatio(splitNode, direction, ratio)
    {
        splitNode.style.setProperty("--split-ratio", String(ratio));
        if(direction === "vertical")
            splitNode.style.gridTemplateColumns = `minmax(0, ${ratio}fr) minmax(0, ${100 - ratio}fr)`;
        else
            splitNode.style.gridTemplateRows = `minmax(0, ${ratio}fr) minmax(0, ${100 - ratio}fr)`;
    },

    refreshAllPaneGeometry()
    {
        main.getAllPanes().forEach((pane) =>
        {
            const elements = main.getPaneElements(pane);
            main.drawGrid(elements);
            if(main.getPaneBackground(pane))
                main.withPane(pane, () => main.addConnections());
        });
        main.updateWorkspaceConnections();
    },

    getFirstPane(node)
    {
        if(!node)
            return null;
        if(node.classList && node.classList.contains("main_pane"))
            return node;
        return node.querySelector ? node.querySelector(".main_pane") : null;
    },

    closePane(source=null)
    {
        const pane = main.getPaneFromSource(source);
        if(!pane || !main.workspace)
            return;

        const allPanes = main.getAllPanes();
        if(allPanes.length <= 1)
            return;

        const splitNode = pane.parentElement;
        if(!splitNode || !splitNode.classList || !splitNode.classList.contains("split_node"))
            return;

        const sibling = Array.from(splitNode.children).find((child) => child !== pane && !child.classList.contains("split_resize_handle"));
        if(!sibling)
            return;

        const nextActivePane = main.getFirstPane(sibling);
        const splitParent = splitNode.parentElement;
        if(!splitParent)
            return;

        splitNode.replaceWith(sibling);
        if(nextActivePane)
            main.activatePane(nextActivePane, false);
        requestAnimationFrame(() =>
        {
            main.refreshAllPaneGeometry();
            main.savePaneLayout();
        });
    },

    // Reconnect overlay state.
    createReconnectOverlay()
    {
        if(main.reconnect_overlay && main.reconnect_overlay.parentElement)
            return;

        const overlay = document.createElement("div");
        overlay.className = "reconnect-overlay";
        overlay.setAttribute("aria-live", "polite");
        overlay.innerHTML = `
            <div class="reconnect-overlay-content">
                <span class="status-spinner reconnect-spinner" aria-hidden="true"></span>
                <span class="reconnect-overlay-label">waiting to reconnect</span>
            </div>
        `;
        main.main.appendChild(overlay);
        main.reconnect_overlay = overlay;
    },

    showReconnectOverlay()
    {
        if(!main.main)
            return;
        if(!main.reconnect_overlay)
            main.createReconnectOverlay();
        if(main.reconnect_overlay)
            main.reconnect_overlay.classList.add("visible");
    },

    hideReconnectOverlay()
    {
        if(main.reconnect_overlay)
            main.reconnect_overlay.classList.remove("visible");
    },

    // Context and component/widget menus.
    stopMenuPointerPropagation(menu)
    {
        if(!menu)
            return;
        menu.addEventListener("mousedown", function(evt)
        {
            evt.stopPropagation();
        }, true);
    },

    hideMenu(menu, visibleProperty, onHide=null)
    {
        if(!menu)
            return;
        menu.style.display = "none";
        main[visibleProperty] = false;
        if(onHide)
            onHide();
    },

    showMenuAt(menu, visibleProperty, clientX, clientY, onShow=null)
    {
        if(!menu)
            return;

        if(onShow)
            onShow();

        menu.style.display = "block";
        const rect = menu.getBoundingClientRect();
        const maxLeft = Math.max(0, window.innerWidth - rect.width - 4);
        const maxTop = Math.max(0, window.innerHeight - rect.height - 4);
        const left = Math.max(0, Math.min(clientX, maxLeft));
        const top = Math.max(0, Math.min(clientY, maxTop));
        menu.style.left = `${left}px`;
        menu.style.top = `${top}px`;
        main[visibleProperty] = true;
    },

    hideAllMenus(exceptMenu=null)
    {
        if(main.context_menu && main.context_menu !== exceptMenu)
            main.hideContextMenu();
        if(main.component_color_menu && main.component_color_menu !== exceptMenu)
            main.hideComponentColorMenu();
        if(main.widget_menu && main.widget_menu !== exceptMenu)
            main.hideWidgetMenu();
    },

    setTopChromeVisible(visible)
    {
        document.body.classList.toggle("top_chrome_hidden", !visible);
    },

    toggleTopChrome()
    {
        const hidden = document.body.classList.toggle("top_chrome_hidden");
        return !hidden;
    },

    isTrueAttributeValue(value)
    {
        if(value === true)
            return true;
        if(typeof value === "number")
            return value !== 0;
        if(typeof value !== "string")
            return false;

        const normalized = value.trim().toLowerCase();
        return normalized === "true" || normalized === "1" || normalized === "yes" || normalized === "on";
    },

    applyStartupTopChromeVisibility()
    {
        const hideToolbar = network && network.network ? main.isTrueAttributeValue(network.network.hide_toolbar) : false;
        main.setTopChromeVisible(!hideToolbar);
    },

    registerGlobalMenuDismissHandlers()
    {
        if(main.global_menu_handlers_registered)
            return;
        main.global_menu_handlers_registered = true;

        document.addEventListener("mousedown", function(evt)
        {
            if(
                evt.target &&
                evt.target.closest &&
                evt.target.closest(".main-context-menu")
            )
                return;
            main.hideAllMenus();
        }, true);

        document.addEventListener("keydown", function(evt)
        {
            if(evt.key === "Escape")
                main.hideAllMenus();
        }, true);

        document.addEventListener("scroll", function(evt)
        {
            const target = evt ? evt.target : null;
            if(
                target &&
                target.closest &&
                target.closest(".main-context-menu")
            )
                return;
            main.hideAllMenus();
        }, true);

        window.addEventListener("blur", function()
        {
            main.hideAllMenus();
        }, false);
    },

    createContextMenu()
    {
        if(main.context_menu && main.context_menu.parentElement)
            return;

        const menu = document.createElement("div");
        menu.className = "main-context-menu";
        menu.innerHTML = `
            <button type="button" class="main-context-menu-item" data-choice="Module">Module</button>
            <button type="button" class="main-context-menu-item" data-choice="Group">Group</button>
            <button type="button" class="main-context-menu-item" data-choice="Input">Input</button>
            <button type="button" class="main-context-menu-item" data-choice="Output">Output</button>
            <button type="button" class="main-context-menu-item" data-choice="Widget">Widget</button>
        `;
        document.body.appendChild(menu);
        main.context_menu = menu;
        main.stopMenuPointerPropagation(menu);
        main.registerGlobalMenuDismissHandlers();

        menu.addEventListener("click", function(evt)
        {
            const button = evt.target.closest(".main-context-menu-item");
            if(!button)
                return;
            main.onContextMenuChoice(button.dataset.choice || "");
            main.hideContextMenu();
            evt.preventDefault();
            evt.stopPropagation();
        }, false);
    },

    showContextMenuAt(clientX, clientY)
    {
        if(!main.context_menu)
            return;

        main.hideAllMenus(main.context_menu);
        main.showMenuAt(main.context_menu, "context_menu_visible", clientX, clientY);
    },

    hideContextMenu()
    {
        main.hideMenu(main.context_menu, "context_menu_visible");
    },

    createComponentColorMenu()
    {
        if(main.component_color_menu && main.component_color_menu.parentElement)
            return;

        const menu = document.createElement("div");
        menu.className = "main-context-menu";
        menu.innerHTML = `
            <div class="main-context-submenu component-class-submenu">
                <button type="button" class="main-context-menu-item main-context-submenu-trigger">class</button>
                <div class="main-context-submenu-panel main-context-class-submenu-panel" role="menu" aria-label="Class"></div>
            </div>
            <div class="main-context-submenu">
                <button type="button" class="main-context-menu-item main-context-submenu-trigger">color</button>
                <div class="main-context-submenu-panel" role="menu" aria-label="Color">
                    <button type="button" class="main-context-color-swatch" data-color="red" title="red" aria-label="red"><span class="main-context-color-chip" style="background:#d84a4a"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="orange" title="orange" aria-label="orange"><span class="main-context-color-chip" style="background:#e58e3a"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="yellow" title="yellow" aria-label="yellow"><span class="main-context-color-chip" style="background:#efe26a"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="green" title="green" aria-label="green"><span class="main-context-color-chip" style="background:#58a663"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="blue" title="blue" aria-label="blue"><span class="main-context-color-chip" style="background:#4f79c8"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="purple" title="purple" aria-label="purple"><span class="main-context-color-chip" style="background:#7f59b3"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="pink" title="pink" aria-label="pink"><span class="main-context-color-chip" style="background:#d77db2"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="white" title="white" aria-label="white"><span class="main-context-color-chip" style="background:#f5f5f5"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="black" title="black" aria-label="black"><span class="main-context-color-chip" style="background:#1a1a1a"></span></button>
                </div>
            </div>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="duplicate">Duplicate</button>
            <button type="button" class="main-context-menu-item" data-action="delete">Delete</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="rename">Rename</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="show-inspector">Show Inspector...</button>
            <div class="main-context-menu-separator component-documentation-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="show-documentation">Show documentation...</button>
        `;
        document.body.appendChild(menu);
        main.component_color_menu = menu;
        main.component_class_submenu = menu.querySelector(".component-class-submenu");
        main.component_class_submenu_panel = menu.querySelector(".main-context-class-submenu-panel");
        main.stopMenuPointerPropagation(menu);

        menu.addEventListener("click", function(evt)
        {
            const classButton = evt.target.closest(".main-context-class-item");
            if(classButton)
            {
                main.setComponentClass(classButton.dataset.className || "");
                main.hideComponentColorMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            const button = evt.target.closest(".main-context-color-swatch");
            if(button)
            {
                main.setComponentColor(button.dataset.color || "");
                main.hideComponentColorMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            const actionButton = evt.target.closest(".main-context-menu-item[data-action]");
            if(actionButton)
            {
                main.handleComponentMenuAction(actionButton.dataset.action || "");
                main.hideComponentColorMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            main.hideComponentColorMenu();
        }, false);
    },

    createWidgetMenu()
    {
        if(main.widget_menu && main.widget_menu.parentElement)
            main.widget_menu.remove();

        const menu = document.createElement("div");
        menu.className = "main-context-menu";
        menu.innerHTML = `
            <div class="main-context-submenu widget-class-submenu">
                <button type="button" class="main-context-menu-item main-context-submenu-trigger">class</button>
                <div class="main-context-submenu-panel main-context-class-submenu-panel" role="menu" aria-label="Widget Class"></div>
            </div>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="toggle-title">Hide Title</button>
            <button type="button" class="main-context-menu-item" data-action="toggle-frame">Hide Frame</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="bring-to-front">Bring to Front</button>
            <button type="button" class="main-context-menu-item" data-action="bring-to-back">Send to Back</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="duplicate">Duplicate</button>
            <button type="button" class="main-context-menu-item" data-action="delete">Delete</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="rename">Rename</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="show-inspector">Show Inspector...</button>
        `;
        document.body.appendChild(menu);
        main.widget_menu = menu;
        main.widget_class_submenu = menu.querySelector(".widget-class-submenu");
        main.widget_class_submenu_panel = menu.querySelector(".main-context-class-submenu-panel");

        menu.addEventListener("mousedown", function(evt)
        {
            evt.stopPropagation();
        }, true);

        menu.addEventListener("click", function(evt)
        {
            const classButton = evt.target.closest(".main-context-class-item");
            if(classButton)
            {
                main.setWidgetClass(classButton.dataset.className || "");
                main.hideWidgetMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            const actionButton = evt.target.closest(".main-context-menu-item[data-action]");
            if(actionButton)
            {
                main.handleWidgetMenuAction(actionButton.dataset.action || "");
                main.hideWidgetMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            main.hideWidgetMenu();
        }, false);
    },

    handleComponentMenuAction(action)
    {
        if(!main.edit_mode)
            return;
        const fullName = main.component_color_target;
        const item = fullName ? network.dict[fullName] : null;
        if(!fullName || !item)
            return;
        if(!selector.selected_foreground.includes(fullName))
            selector.selectItems([fullName], selector.selected_background);
        switch((action || "").toLowerCase())
        {
            case "duplicate":
                main.duplicateSelectedComponents(false);
                break;
            case "delete":
                main.deleteComponent();
                break;
            case "rename":
                main.startInlineNameEditForComponent(fullName);
                break;
            case "show-inspector":
                if(inspector && typeof inspector.showInspectorForSelection === "function")
                    inspector.showInspectorForSelection();
                if(inspector && typeof inspector.showComponent === "function")
                    inspector.showComponent();
                break;
            case "show-documentation":
                if(item && item._tag === "module" && inspector && typeof inspector.showLibraryForClass === "function")
                    inspector.showLibraryForClass(item.class);
                break;
        }
    },

    handleWidgetMenuAction(action)
    {
        if(!main.edit_mode)
            return;
        const fullName = main.widget_menu_target;
        const item = fullName ? network.dict[fullName] : null;
        if(!fullName || !item || item._tag !== "widget")
            return;
        if(!selector.selected_foreground.includes(fullName))
            selector.selectItems([fullName], selector.selected_background);
        switch((action || "").toLowerCase())
        {
            case "toggle-title":
                main.toggleWidgetParameter("show_title");
                break;
            case "toggle-frame":
                main.toggleWidgetParameter("show_frame");
                break;
            case "bring-to-front":
                main.reorderWidget(fullName, "front");
                break;
            case "bring-to-back":
                main.reorderWidget(fullName, "back");
                break;
            case "duplicate":
                main.duplicateSelectedComponents(false);
                break;
            case "delete":
                main.deleteComponent();
                break;
            case "rename":
                if(!item.show_title)
                {
                    if(inspector && typeof inspector.showComponent === "function")
                        inspector.showComponent();
                    if(inspector && typeof inspector.showInspectorForSelection === "function")
                        inspector.showInspectorForSelection();
                    setTimeout(function() { inspector.focusEditableField("title"); }, 0);
                }
                else
                    main.startInlineNameEditForComponent(fullName);
                break;
            case "show-inspector":
                if(inspector && typeof inspector.showInspectorForSelection === "function")
                    inspector.showInspectorForSelection();
                if(inspector && typeof inspector.showComponent === "function")
                    inspector.showComponent();
                break;
        }
    },

    populateWidgetClassSubmenu(widgetFullName)
    {
        if(!main.widget_class_submenu || !main.widget_class_submenu_panel)
            return;

        const item = network.dict[widgetFullName];
        if(!item || item._tag !== "widget")
        {
            main.widget_class_submenu.style.display = "none";
            main.widget_class_submenu_panel.innerHTML = "";
            return;
        }

        main.widget_class_submenu.style.display = "";
        main.widget_class_submenu_panel.innerHTML = "";
        for(const className of widget_classes)
        {
            const b = document.createElement("button");
            b.type = "button";
            b.className = "main-context-class-item";
            if(className === item.class)
                b.classList.add("selected");
            b.dataset.className = className;
            b.textContent = className;
            main.widget_class_submenu_panel.appendChild(b);
        }

        const titleToggle = main.widget_menu.querySelector('.main-context-menu-item[data-action="toggle-title"]');
        const frameToggle = main.widget_menu.querySelector('.main-context-menu-item[data-action="toggle-frame"]');
        if(titleToggle)
            titleToggle.textContent = item.show_title ? "Hide Title" : "Show Title";
        if(frameToggle)
            frameToggle.textContent = item.show_frame ? "Hide Frame" : "Show Frame";
    },

    getComponentColorTargetFromElement(element)
    {
        if(!element || !element.closest)
            return null;

        let gi = null;
        const titleCell = element.closest("td.title");
        if(titleCell)
            gi = titleCell.closest(".gi");
        else
            gi = element.closest(".group_input, .group_output");

        if(!gi || !gi.dataset || !gi.dataset.name)
            return null;

        const fullName = gi.dataset.name;
        const item = network.dict[fullName];
        if(!item)
            return null;
        if(!["module", "group", "input", "output"].includes(item._tag))
            return null;

        return fullName;
    },

    showComponentColorMenuAt(clientX, clientY, componentFullName, preferredSubmenu="color", propertyName="color")
    {
        if(!main.component_color_menu || !componentFullName)
            return;

        main.hideAllMenus(main.component_color_menu);
        main.showMenuAt(main.component_color_menu, "component_color_menu_visible", clientX, clientY, function()
        {
            main.populateComponentClassSubmenu(componentFullName);
            main.component_color_target = componentFullName;
            main.component_color_property = propertyName || "color";
            const item = network.dict[componentFullName];
            const documentationButton = main.component_color_menu.querySelector('.main-context-menu-item[data-action="show-documentation"]');
            const documentationSeparator = main.component_color_menu.querySelector('.component-documentation-separator');
            const canShowDocumentation = !!(item && item._tag === "module" && item.class);
            if(documentationButton)
                documentationButton.style.display = canShowDocumentation ? "" : "none";
            if(documentationSeparator)
                documentationSeparator.style.display = canShowDocumentation ? "" : "none";
            if(preferredSubmenu === "class")
            {
                main.component_color_menu.classList.add("open-class-submenu");
                main.component_color_menu.classList.add("open-class-only");
                main.component_color_menu.classList.remove("open-color-only");
            }
            else if(preferredSubmenu === "color-only")
            {
                main.component_color_menu.classList.remove("open-class-submenu");
                main.component_color_menu.classList.remove("open-class-only");
                main.component_color_menu.classList.add("open-color-only");
            }
            else
            {
                main.component_color_menu.classList.remove("open-class-submenu");
                main.component_color_menu.classList.remove("open-class-only");
                main.component_color_menu.classList.remove("open-color-only");
            }
        });
    },

    hideComponentColorMenu()
    {
        main.hideMenu(main.component_color_menu, "component_color_menu_visible", function()
        {
            main.component_color_menu.classList.remove("open-class-submenu");
            main.component_color_menu.classList.remove("open-class-only");
            main.component_color_menu.classList.remove("open-color-only");
            main.component_color_target = null;
            main.component_color_property = "color";
        });
    },

    showWidgetMenuAt(clientX, clientY, widgetFullName)
    {
        if(!main.widget_menu || !widgetFullName)
            return;

        main.hideAllMenus(main.widget_menu);
        main.showMenuAt(main.widget_menu, "widget_menu_visible", clientX, clientY, function()
        {
            main.populateWidgetClassSubmenu(widgetFullName);
            main.widget_menu_target = widgetFullName;
        });
    },

    hideWidgetMenu()
    {
        main.hideMenu(main.widget_menu, "widget_menu_visible", function()
        {
            main.widget_menu_target = null;
        });
    },

    setWidgetClass(className)
    {
        const fullName = main.widget_menu_target;
        if(!fullName || !className)
            return;
        const item = network.dict[fullName];
        if(!item || item._tag !== "widget")
            return;

        network.changeWidgetClass(fullName, className);
        selector.selectItems([fullName], null, false, false, true);
        main.syncLockedMirrorItemProperties(fullName, ["class"]);
        controller.setTainted(true, "setWidgetClass");
    },

    toggleWidgetParameter(parameterName)
    {
        const fullName = main.widget_menu_target;
        if(!fullName || !parameterName)
            return;
        const item = network.dict[fullName];
        const widgetFrame = main.getPaneElement(fullName);
        const widgetElement = main.getFrameWidget(widgetFrame);
        if(!item || item._tag !== "widget" || !widgetElement)
            return;

        item[parameterName] = !widgetElement.toBool(item[parameterName]);
        widgetElement.parameters[parameterName] = item[parameterName];
        try
        {
            widgetElement.updateAll();
        }
        catch(err)
        {
            console.log(err);
        }
        if(inspector && typeof inspector.showInspectorForSelection === "function")
            inspector.showInspectorForSelection();
        main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
        main.syncLockedMirrorItemProperties(fullName, [parameterName]);
        controller.setTainted(true, "toggleWidgetParameter");
    },

    reorderWidget(fullName, direction)
    {
        if(!fullName)
            return;
        const group = network.dict[selector.selected_background];
        const item = network.dict[fullName];
        if(!group || !item || item._tag !== "widget" || !Array.isArray(group.widgets))
            return;

        const index = group.widgets.findIndex((widget) => widget && widget.name === item.name);
        if(index < 0)
            return;

        const [widget] = group.widgets.splice(index, 1);
        if(direction === "back")
            group.widgets.unshift(widget);
        else
            group.widgets.push(widget);

        network.rebuildDict();
        selector.selectItems([fullName], selector.selected_background, false, false, true);
        controller.setTainted(true, "reorderWidget");
    },

    setComponentColor(color)
    {
        const fullName = main.component_color_target;
        const propertyName = main.component_color_property || "color";
        if(!fullName || !color)
            return;
        const item = network.dict[fullName];
        if(!item || !["module", "group", "input", "output", "connection"].includes(item._tag))
            return;
        item[propertyName] = color;
        if(item._tag == "connection")
        {
            controller.setTainted(true, "setComponentColor connection");
            if(selector.selected_connection)
                selector.selectConnection(selector.selected_connection);
            if(inspector && typeof inspector.showInspectorForSelection === "function")
                inspector.showInspectorForSelection();
            main.addConnections();
            main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
            return;
        }
        if(selector.selected_background != null)
            main.selectItem(selector.selected_foreground, selector.selected_background);
        if(inspector && typeof inspector.showInspectorForSelection === "function")
            inspector.showInspectorForSelection();
        main.syncLockedMirrorItemProperties(fullName, [propertyName]);
        controller.setTainted(true, "setComponentColor");
    },

    populateComponentClassSubmenu(componentFullName)
    {
        if(!main.component_class_submenu || !main.component_class_submenu_panel)
            return;

        const item = network.dict[componentFullName];
        if(!item || item._tag !== "module" || !Array.isArray(network.classes) || network.classes.length === 0)
        {
            main.component_class_submenu.style.display = "none";
            main.component_class_submenu_panel.innerHTML = "";
            return;
        }

        main.component_class_submenu.style.display = "";
        main.component_class_submenu_panel.innerHTML = "";
        for(const className of network.classes)
        {
            const b = document.createElement("button");
            b.type = "button";
            b.className = "main-context-class-item";
            if(className === item.class)
                b.classList.add("selected");
            b.dataset.className = className;
            b.textContent = className;
            main.component_class_submenu_panel.appendChild(b);
        }
    },

    setComponentClass(className)
    {
        const fullName = main.component_color_target;
        if(!fullName || !className)
            return;
        const item = network.dict[fullName];
        if(!item || item._tag !== "module")
            return;

        network.changeModuleClass(fullName, className);
        selector.selectItems([fullName], null, false, false, true);
        main.syncLockedMirrorItemProperties(fullName, ["class", "inputs", "outputs", "parameters"]);
        controller.setTainted(true, "setComponentClass");
    },

    openComponentColorMenuFromButton(evt)
    {
        if(!main.edit_mode)
            return;
        evt.preventDefault();
        evt.stopPropagation();
        const button = evt.currentTarget || evt.target.closest(".module-title-menu-button");
        if(!button)
            return;
        const fullName = button.dataset.component || (button.closest(".gi") ? button.closest(".gi").dataset.name : null);
        if(!fullName)
            return;
        const r = button.getBoundingClientRect();
        main.showComponentColorMenuAt(r.right + 2, r.top, fullName, "color");
    },

    openComponentClassMenuFromButton(evt)
    {
        if(!main.edit_mode)
            return;
        evt.preventDefault();
        evt.stopPropagation();
        const button = evt.currentTarget || evt.target.closest(".module-class-menu-button");
        if(!button)
            return;
        const fullName = button.dataset.component || (button.closest(".gi.module") ? button.closest(".gi.module").dataset.name : null);
        if(!fullName)
            return;
        const r = button.getBoundingClientRect();
        main.showComponentColorMenuAt(r.right + 2, r.top, fullName, "class");
    },

    openWidgetMenuFromButton(evt)
    {
        if(!main.edit_mode)
            return;
        evt.preventDefault();
        evt.stopPropagation();
        const button = evt.currentTarget || evt.target.closest(".widget-title-menu-button");
        if(!button)
            return;
        const fullName = button.dataset.component || (button.closest(".gi.widget") ? button.closest(".gi.widget").dataset.name : null);
        if(!fullName)
            return;
        const r = button.getBoundingClientRect();
        main.showWidgetMenuAt(r.right + 2, r.top, fullName);
    },

    getSnappedBackgroundPositionFromEvent(evt)
    {
        const viewRect = main.view.getBoundingClientRect();
        let x = evt.clientX - viewRect.left;
        let y = evt.clientY - viewRect.top;
        if(x < 0) x = 0;
        if(y < 0) y = 0;
        const g = main.grid_spacing || 1;
        x = g * Math.round(x / g);
        y = g * Math.round(y / g);
        return {x: main.paneXToModelX(x), y};
    },

    isBackgroundEventTarget(target)
    {
        if(!target)
            return false;
        if(target === main.view || target === main.selection_box)
            return true;
        if(target.closest)
        {
            if(target.closest(".gi"))
                return false;
            if(target.closest(".connection_line"))
                return false;
            if(target.closest(".o_spot") || target.closest(".i_spot"))
                return false;
        }
        return !!(main.view && main.view.contains(target));
    },

    showBackgroundContextMenu(evt)
    {
        if(!main.edit_mode)
            return;

        const isModuleTitleRow = !!(evt.target && evt.target.closest && evt.target.closest(".gi.module td.title"));
        if(isModuleTitleRow)
            return;

        const colorTarget = main.getComponentColorTargetFromElement(evt.target);
        if(colorTarget)
        {
            evt.preventDefault();
            evt.stopPropagation();
            main.showComponentColorMenuAt(evt.clientX, evt.clientY, colorTarget, "color");
            return;
        }

        if(!main.isBackgroundEventTarget(evt.target))
            return;

        evt.preventDefault();
        evt.stopPropagation();
        main.context_menu_position = main.getSnappedBackgroundPositionFromEvent(evt);
        main.showContextMenuAt(evt.clientX, evt.clientY);
    },

    onContextMenuChoice(choice)
    {
        if(main.context_menu_position)
        {
            main.new_position_x = main.context_menu_position.x;
            main.new_position_y = main.context_menu_position.y;
        }

        switch(choice)
        {
            case "Module":
                main.newModule();
                break;
            case "Group":
                main.newGroup();
                break;
            case "Input":
                main.newInput();
                break;
            case "Output":
                main.newOutput();
                break;
            case "Widget":
                main.newWidget();
                break;
        }

        main.context_menu_position = null;
    },

    ensureSelectionBox()
    {
        if(!main.view)
            return;
        if(main.selection_box && main.selection_box.parentElement === main.view)
            return;
        if(!main.selection_box)
        {
            main.selection_box = document.createElement("div");
            main.selection_box.className = "selection-rectangle";
        }
        main.selection_box.style.display = "none";
        main.view.appendChild(main.selection_box);
    },

    startBackgroundSelection(evt)
    {
        main.hideContextMenu();
        main.hideComponentColorMenu();
        if(main.inline_name_edit)
            main.finishInlineNameEdit(false);
        if(evt.button !== 0)
            return;
        if(!main.isBackgroundEventTarget(evt.target))
            return;

        if(!main.edit_mode)
        {
            selector.selectBackgroundAndReveal();
            return;
        }

        main.ensureSelectionBox();
        main.selection_drag_active = true;
        main.selection_drag_shift = evt.shiftKey;
        main.selection_base_foreground = [...selector.selected_foreground];
        main.selection_start_x = evt.clientX;
        main.selection_start_y = evt.clientY;
        main.selection_moved = false;

        const viewRect = main.view.getBoundingClientRect();
        main.selection_box.style.left = `${Math.max(0, evt.clientX - viewRect.left)}px`;
        main.selection_box.style.top = `${Math.max(0, evt.clientY - viewRect.top)}px`;
        main.selection_box.style.width = "0px";
        main.selection_box.style.height = "0px";
        main.selection_box.style.display = "block";

        document.addEventListener("mousemove", main.updateBackgroundSelection, true);
        document.addEventListener("mouseup", main.finishBackgroundSelection, true);
        evt.preventDefault();
        evt.stopPropagation();
    },

    collectBackgroundSelection(left, top, width, height)
    {
        const x1 = left;
        const y1 = top;
        const x2 = left + width;
        const y2 = top + height;
        const viewRect = main.view.getBoundingClientRect();
        const selected = [];

        for(const element of main.view.querySelectorAll(".gi"))
        {
            const r = element.getBoundingClientRect();
            const ex1 = r.left - viewRect.left;
            const ey1 = r.top - viewRect.top;
            const ex2 = ex1 + r.width;
            const ey2 = ey1 + r.height;

            const intersects = !(ex2 < x1 || ex1 > x2 || ey2 < y1 || ey1 > y2);
            if(intersects && element.dataset.name)
                selected.push(element.dataset.name);
        }

        return selected;
    },

    applyBackgroundSelection(selected, toggle_selection)
    {
        if(toggle_selection)
        {
            const base = new Set(main.selection_base_foreground || []);
            for(const name of selected)
            {
                if(base.has(name))
                    base.delete(name);
                else
                    base.add(name);
            }
            selector.selectItemsAndReveal(Array.from(base));
            return;
        }

        if(selected.length === 0)
        {
            selector.selectBackgroundAndReveal();
            return;
        }

        selector.selectItemsAndReveal(selected);
    },

    updateBackgroundSelection(evt)
    {
        if(!main.selection_drag_active || !main.selection_box)
            return;

        const viewRect = main.view.getBoundingClientRect();
        const x1 = Math.max(0, Math.min(main.selection_start_x - viewRect.left, main.view.clientWidth));
        const y1 = Math.max(0, Math.min(main.selection_start_y - viewRect.top, main.view.clientHeight));
        const x2 = Math.max(0, Math.min(evt.clientX - viewRect.left, main.view.clientWidth));
        const y2 = Math.max(0, Math.min(evt.clientY - viewRect.top, main.view.clientHeight));

        const left = Math.min(x1, x2);
        const top = Math.min(y1, y2);
        const width = Math.abs(x2 - x1);
        const height = Math.abs(y2 - y1);

        if(width > 2 || height > 2)
            main.selection_moved = true;

        main.selection_box.style.left = `${left}px`;
        main.selection_box.style.top = `${top}px`;
        main.selection_box.style.width = `${width}px`;
        main.selection_box.style.height = `${height}px`;
    },

    finishBackgroundSelection(evt)
    {
        if(!main.selection_drag_active)
            return;

        document.removeEventListener("mousemove", main.updateBackgroundSelection, true);
        document.removeEventListener("mouseup", main.finishBackgroundSelection, true);

        if(main.selection_box)
            main.selection_box.style.display = "none";

        if(!main.selection_moved)
        {
            const toggle_selection = main.selection_drag_shift || evt.shiftKey;
            if(!toggle_selection)
                selector.selectBackgroundAndReveal();
            main.selection_drag_active = false;
            main.selection_base_foreground = [];
            return;
        }

        const viewRect = main.view.getBoundingClientRect();
        const x1 = Math.max(0, Math.min(main.selection_start_x - viewRect.left, main.view.clientWidth));
        const y1 = Math.max(0, Math.min(main.selection_start_y - viewRect.top, main.view.clientHeight));
        const x2 = Math.max(0, Math.min(evt.clientX - viewRect.left, main.view.clientWidth));
        const y2 = Math.max(0, Math.min(evt.clientY - viewRect.top, main.view.clientHeight));
        const left = Math.min(x1, x2);
        const top = Math.min(y1, y2);
        const width = Math.abs(x2 - x1);
        const height = Math.abs(y2 - y1);
        const toggle_selection = main.selection_drag_shift || evt.shiftKey;
        const selected = main.collectBackgroundSelection(left, top, width, height);
        main.applyBackgroundSelection(selected, toggle_selection);

        main.selection_drag_active = false;
        main.selection_base_foreground = [];
        evt.preventDefault();
        evt.stopPropagation();
    },

    ensureGridCanvasSize(pane=main.active_pane)
    {
        const gridCanvas = pane ? pane.grid_canvas : null;
        const componentLayer = pane ? pane.component_layer : null;
        if(!gridCanvas || !main.main || !componentLayer)
            return;
        const dpr = window.devicePixelRatio || 1;
        const minSize = 3000;
        const targetWidth = Math.max(minSize, main.main.clientWidth, componentLayer.clientWidth);
        const targetHeight = Math.max(minSize, main.main.clientHeight, componentLayer.clientHeight);
        const backingWidth = Math.floor(targetWidth * dpr);
        const backingHeight = Math.floor(targetHeight * dpr);
        if(gridCanvas.width !== backingWidth)
            gridCanvas.width = backingWidth;
        if(gridCanvas.height !== backingHeight)
            gridCanvas.height = backingHeight;
        gridCanvas.style.width = `${targetWidth}px`;
        gridCanvas.style.height = `${targetHeight}px`;
        pane.grid_canvas_css_width = targetWidth;
        pane.grid_canvas_css_height = targetHeight;
        pane.grid_canvas_dpr = dpr;
    },

    // Background interaction, grid drawing, and canvas layout.
    drawGrid(pane=main.active_pane)
    {
        main.ensureGridCanvasSize(pane);
        const gridCanvas = pane ? pane.grid_canvas : null;
        if(!gridCanvas)
            return;
        const ctx = gridCanvas.getContext("2d");
        const width = pane.grid_canvas_css_width || gridCanvas.width;
        const height = pane.grid_canvas_css_height || gridCanvas.height;
        const dpr = pane.grid_canvas_dpr || 1;
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        ctx.fillStyle = "white";
        ctx.fillRect(0, 0, width, height);
        ctx.lineWidth = 1 / dpr;
        ctx.strokeStyle = "gray";
        const maxDim = Math.max(width, height);
        for(let x = 0; x < maxDim; x += main.grid_spacing)
        {
            const xx = x;
            ctx.beginPath();
            ctx.moveTo(xx, 0);
            ctx.lineTo(xx, height);
            ctx.moveTo(0,xx);
            ctx.lineTo(width,xx);
            ctx.stroke();
        }
    },

    increaseGrid()
    {
        main.grid_spacing = main.grid_spacing*2;
        if(main.grid_spacing > 192)
            main.grid_spacing = 192;
        main.storeBackgroundGridSpacing();
        main.drawGrid();
    },

    decreaseGrid()
    {
        main.grid_spacing = main.grid_spacing/2;
        if(main.grid_spacing < 8)
            main.grid_spacing = 8;
        main.storeBackgroundGridSpacing();
        main.drawGrid();
    },

    centerComponents()
    {
        const g = network.dict[selector.selected_background];
        const sum_x = 0;
        const sum_y = 0;
        const n = 0;
        const comps = [...g.groups||[], ...g.modules||[], ...g.inputs||[], ...g.outputs||[], ...g.widgets||[]];
        for(let c of comps)
        {
            sum_x += parseInt(c._x);
            sum_y += parseInt(c._y);
            n += 1;
        }

        if(n==0)
            return;

        const old_center_x = sum_x/n;
        const old_center_y = sum_y/n;    
        const new_center_x = main.main.offsetWidth/2;
        const new_center_y = main.main.offsetHeight/2;
        const dx = new_center_x-old_center_x;
        const dy = new_center_y-old_center_y;

        for(let c of comps)
        {
            c._x = `${parseInt(c._x) + dx}`;
            c._y = `${parseInt(c._y) + dy}`;
        }
        selector.selectItems([], null);
    },

    getArrangeItems(group)
    {
        const items = [];
        const pushItem = function(localName, object)
        {
            if(!localName || !object)
                return;
            const fullName = `${selector.selected_background}.${localName}`;
            const element = main.getPaneElement(fullName);
            let width = element ? element.offsetWidth : 0;
            let height = element ? element.offsetHeight : 0;
            if(width <= 0 || height <= 0)
            {
                if(object._tag == "widget")
                {
                    width = parseInt(object.width) || 200;
                    height = parseInt(object.height) || 140;
                }
                else if(object._tag == "input" || object._tag == "output")
                {
                    width = 140;
                    height = 26;
                }
                else
                {
                    const inCount = (object.inputs || []).length;
                    const outCount = (object.outputs || []).length;
                    width = 220;
                    height = 34 + Math.max(inCount, outCount) * 20;
                }
            }
            items.push({
                localName,
                fullName,
                object,
                width: Math.max(60, width),
                height: Math.max(20, height),
                inDegree: 0,
                outDegree: 0,
                layer: 0
            });
        };

        for(const g of group.groups || [])
            pushItem(g.name, g);
        for(const m of group.modules || [])
            pushItem(m.name, m);
        for(const i of group.inputs || [])
            pushItem(i.name, i);
        for(const o of group.outputs || [])
            pushItem(o.name, o);
        // Widgets are intentionally excluded from auto-arrange.
        // They keep their current positions.
        return items;
    },

    getBackgroundAutoRoutingGroup()
    {
        const background = selector.selected_background;
        const group = background ? network.dict[background] : null;
        if(!group)
            return null;
        if(group.auto_routing === undefined)
            group.auto_routing = false;
        return group;
    },

    updateAutoRoutingButtonState()
    {
        if(!main.auto_routing_toggle_button)
            return;
        const group = main.getBackgroundAutoRoutingGroup();
        const enabled = !!(group && group.auto_routing);
        main.auto_routing_toggle_button.classList.toggle("selected", enabled);
        main.auto_routing_toggle_button.setAttribute("aria-pressed", enabled ? "true" : "false");
        main.auto_routing_toggle_button.title = enabled ? "Disable automatic routing" : "Enable automatic routing";
    },

    getBackgroundGridGroup()
    {
        const background = selector.selected_background;
        return background ? network.dict[background] : null;
    },

    applyBackgroundGridSpacing()
    {
        const group = main.getBackgroundGridGroup();
        if(!group)
            return;
        const parsed = parseInt(group.grid_spacing, 10);
        const nextGridSpacing = Number.isFinite(parsed) ? Math.min(192, Math.max(8, parsed)) : 24;
        group.grid_spacing = nextGridSpacing;
        if(main.grid_spacing === nextGridSpacing)
            return;
        main.grid_spacing = nextGridSpacing;
        main.drawGrid();
    },

    storeBackgroundGridSpacing()
    {
        const group = main.getBackgroundGridGroup();
        if(!group)
            return;
        const nextGridSpacing = Math.min(192, Math.max(8, parseInt(main.grid_spacing, 10) || 24));
        if(group.grid_spacing === nextGridSpacing)
            return;
        group.grid_spacing = nextGridSpacing;
        controller.setTainted(true, "storeBackgroundGridSpacing");
        if(inspector && typeof inspector.showInspectorForSelection === "function" && selector.selected_foreground.length === 0)
            inspector.showInspectorForSelection();
    },

    toggleBackgroundAutoRouting()
    {
        const group = main.getBackgroundAutoRoutingGroup();
        if(!group)
            return;
        group.auto_routing = !group.auto_routing;
        controller.setTainted(true, "toggleBackgroundAutoRouting");
        main.updateAutoRoutingButtonState();
        if(inspector && typeof inspector.showInspectorForSelection === "function" && selector.selected_foreground.length === 0)
            inspector.showInspectorForSelection();
        main.addConnections();
        main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
    },

    arrangeComponents(options = {})
    {
        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group)
            return;

        const nodes = main.getArrangeItems(group);
        if(nodes.length === 0)
            return;

        const nodeMap = new Map(nodes.map((n) => [n.localName, n]));
        const outgoing = new Map(nodes.map((n) => [n.localName, new Set()]));
        const incoming = new Map(nodes.map((n) => [n.localName, new Set()]));

        for(const c of group.connections || [])
        {
            const sourcePath = getStringUpToBracket(c.source || "");
            const targetPath = getStringUpToBracket(c.target || "");
            const sourceLocal = sourcePath.split(".")[0];
            const targetLocal = targetPath.split(".")[0];
            if(!nodeMap.has(sourceLocal) || !nodeMap.has(targetLocal) || sourceLocal == targetLocal)
                continue;
            outgoing.get(sourceLocal).add(targetLocal);
            incoming.get(targetLocal).add(sourceLocal);
        }

        for(const n of nodes)
        {
            n.outDegree = outgoing.get(n.localName).size;
            n.inDegree = incoming.get(n.localName).size;
        }

        const inputNodes = nodes.filter((n) => n.object && n.object._tag === "input");
        const outputNodes = nodes.filter((n) => n.object && n.object._tag === "output");
        const middleCandidates = nodes.filter((n) => !inputNodes.includes(n) && !outputNodes.includes(n));

        const sourceNodes = nodes.filter((n) => n.inDegree === 0 && n.outDegree > 0);
        const sinkNodes = nodes.filter((n) => n.outDegree === 0 && n.inDegree > 0);
        const isolatedNodes = nodes.filter((n) => n.inDegree === 0 && n.outDegree === 0);
        const middleNodes = nodes.filter((n) => n.inDegree > 0 && n.outDegree > 0);

        const queue = [];
        const visited = new Set();
        if(sourceNodes.length > 0)
        {
            for(const s of sourceNodes)
            {
                s.layer = 0;
                queue.push(s.localName);
                visited.add(s.localName);
            }
        }
        else
        {
            for(const n of middleNodes)
            {
                n.layer = 1;
                queue.push(n.localName);
                visited.add(n.localName);
            }
        }

        while(queue.length > 0)
        {
            const u = queue.shift();
            const nodeU = nodeMap.get(u);
            for(const v of outgoing.get(u))
            {
                const nodeV = nodeMap.get(v);
                const nextLayer = nodeU.layer + 1;
                if(nextLayer > nodeV.layer)
                    nodeV.layer = nextLayer;
                if(!visited.has(v))
                {
                    visited.add(v);
                    queue.push(v);
                }
            }
        }

        let maxLayer = 0;
        for(const n of nodes)
            maxLayer = Math.max(maxLayer, n.layer);

        for(const n of sinkNodes)
            n.layer = Math.max(n.layer, maxLayer + 1);
        maxLayer = Math.max(maxLayer + (sinkNodes.length ? 1 : 0), ...nodes.map((n) => n.layer));

        for(const n of isolatedNodes)
            n.layer = Math.max(1, Math.floor(maxLayer / 2));

        // Force all inputs to the first (leftmost) layer.
        for(const n of inputNodes)
            n.layer = 0;

        // Keep non-input/non-output nodes in the middle layers (start at layer 1).
        for(const n of middleCandidates)
            n.layer = Math.max(1, n.layer + 1);

        // Force all outputs to the last (rightmost) layer.
        let maxMiddleLayer = 1;
        for(const n of middleCandidates)
            maxMiddleLayer = Math.max(maxMiddleLayer, n.layer);
        for(const n of outputNodes)
            n.layer = maxMiddleLayer + 1;

        const layers = new Map();
        for(const n of nodes)
        {
            if(!layers.has(n.layer))
                layers.set(n.layer, []);
            layers.get(n.layer).push(n);
        }

        const layerIndices = Array.from(layers.keys()).sort((a,b)=>a-b);
        const getLayerPositionMap = function(layerNodes)
        {
            const m = new Map();
            for(let i = 0; i < layerNodes.length; i++)
                m.set(layerNodes[i].localName, i);
            return m;
        };

        const reorderByNeighborLayer = function(layerIndex, neighborLayerIndex, useIncoming)
        {
            const currentLayer = layers.get(layerIndex);
            const neighborLayer = layers.get(neighborLayerIndex);
            if(!currentLayer || !neighborLayer || currentLayer.length <= 1)
                return;

            const neighborPos = getLayerPositionMap(neighborLayer);
            const currentPos = getLayerPositionMap(currentLayer);
            currentLayer.sort((a, b) => {
                const neighA = useIncoming ? Array.from(incoming.get(a.localName)) : Array.from(outgoing.get(a.localName));
                const neighB = useIncoming ? Array.from(incoming.get(b.localName)) : Array.from(outgoing.get(b.localName));
                const avgA = neighA.length ? neighA.map((k) => neighborPos.get(k)).filter((v)=>v!==undefined).reduce((p,c)=>p+c,0) / neighA.length : currentPos.get(a.localName);
                const avgB = neighB.length ? neighB.map((k) => neighborPos.get(k)).filter((v)=>v!==undefined).reduce((p,c)=>p+c,0) / neighB.length : currentPos.get(b.localName);
                if(avgA !== avgB)
                    return avgA - avgB;
                return currentPos.get(a.localName) - currentPos.get(b.localName);
            });
        };

        // Crossing-minimization sweeps (second pass): top-down and bottom-up.
        for(let iter = 0; iter < 4; iter++)
        {
            for(let i = 1; i < layerIndices.length; i++)
                reorderByNeighborLayer(layerIndices[i], layerIndices[i - 1], true);
            for(let i = layerIndices.length - 2; i >= 0; i--)
                reorderByNeighborLayer(layerIndices[i], layerIndices[i + 1], false);
        }

        const yCenter = new Map();
        for(const layerIndex of layerIndices)
        {
            const layerNodes = layers.get(layerIndex);
            let y = 0;
            const rowGap = Math.max(main.grid_spacing || 24, 20);
            for(const n of layerNodes)
            {
                n.y = y;
                yCenter.set(n.localName, y + n.height / 2);
                y += n.height + rowGap;
            }
            const usedHeight = Math.max(0, y - rowGap);
            const offset = -usedHeight / 2;
            for(const n of layerNodes)
                n.y += offset;
        }

        const colGap = Math.max((main.grid_spacing || 24) * 5, 40);
        let cursorX = 0;
        for(const layerIndex of layerIndices)
        {
            const layerNodes = layers.get(layerIndex);
            const layerWidth = Math.max(...layerNodes.map((n) => n.width));
            for(const n of layerNodes)
                n.x = cursorX;
            cursorX += layerWidth + colGap;
        }

        const edges = [];
        for(const n of nodes)
            for(const t of outgoing.get(n.localName))
                if(nodeMap.has(t))
                    edges.push([n, nodeMap.get(t)]);

        const minGap = Math.max(main.grid_spacing || 24, 20);
        const pushMargin = Math.max(4, Math.round(minGap * 0.2));
        const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));

        // Third pass: vertical optimization to reduce edges crossing component boxes.
        for(let iter = 0; iter < 6; iter++)
        {
            const deltaY = new Map(nodes.map((n) => [n.localName, 0]));

            // 1) Pull toward average neighbor center to shorten edges.
            for(const n of nodes)
            {
                const neigh = [...incoming.get(n.localName), ...outgoing.get(n.localName)]
                    .map((k) => nodeMap.get(k))
                    .filter((x) => !!x);
                if(neigh.length === 0)
                    continue;
                const centerY = n.y + n.height / 2;
                const targetY = neigh.reduce((s, m) => s + (m.y + m.height / 2), 0) / neigh.length;
                deltaY.set(n.localName, deltaY.get(n.localName) + clamp((targetY - centerY) * 0.18, -minGap * 0.6, minGap * 0.6));
            }

            // 2) Repel nodes from connection segments that pass through their boxes.
            for(const [u, v] of edges)
            {
                const x1 = u.x + u.width / 2;
                const y1 = u.y + u.height / 2;
                const x2 = v.x + v.width / 2;
                const y2 = v.y + v.height / 2;
                if(Math.abs(x2 - x1) < 1e-6)
                    continue;

                const left = Math.min(x1, x2);
                const right = Math.max(x1, x2);
                for(const w of nodes)
                {
                    if(w === u || w === v)
                        continue;
                    const wx = w.x + w.width / 2;
                    if(wx <= left || wx >= right)
                        continue;
                    const t = (wx - x1) / (x2 - x1);
                    const yLine = y1 + t * (y2 - y1);
                    const top = w.y - pushMargin;
                    const bottom = w.y + w.height + pushMargin;
                    if(yLine < top || yLine > bottom)
                        continue;

                    const wCenter = w.y + w.height / 2;
                    const dist = Math.max(1, Math.abs(wCenter - yLine));
                    const dir = (wCenter >= yLine) ? 1 : -1;
                    const push = clamp((minGap * 1.1) / dist, 0.2, 1.2) * dir * (minGap * 0.55);
                    deltaY.set(w.localName, deltaY.get(w.localName) + push);
                }
            }

            // 3) Apply per-layer and enforce non-overlap with at least one-grid vertical spacing.
            for(const layerIndex of layerIndices)
            {
                const layerNodes = layers.get(layerIndex);
                if(!layerNodes || layerNodes.length === 0)
                    continue;

                for(const n of layerNodes)
                    n.y += deltaY.get(n.localName);

                layerNodes.sort((a, b) => a.y - b.y);
                let currentY = layerNodes[0].y;
                layerNodes[0].y = currentY;
                for(let i = 1; i < layerNodes.length; i++)
                {
                    const prev = layerNodes[i - 1];
                    const n = layerNodes[i];
                    const minY = prev.y + prev.height + minGap;
                    if(n.y < minY)
                        n.y = minY;
                }

                const layerTop = Math.min(...layerNodes.map((n) => n.y));
                const layerBottom = Math.max(...layerNodes.map((n) => n.y + n.height));
                const offset = -((layerTop + layerBottom) / 2);
                for(const n of layerNodes)
                    n.y += offset;
            }
        }

        const edgeCrossesNode = function(u, v, w)
        {
            if(u === w || v === w)
                return false;
            const x1 = u.x + u.width / 2;
            const y1 = u.y + u.height / 2;
            const x2 = v.x + v.width / 2;
            const y2 = v.y + v.height / 2;

            const left = w.x - pushMargin;
            const right = w.x + w.width + pushMargin;
            const top = w.y - pushMargin;
            const bottom = w.y + w.height + pushMargin;

            const edgeMinX = Math.min(x1, x2);
            const edgeMaxX = Math.max(x1, x2);
            if(edgeMaxX < left || edgeMinX > right)
                return false;

            if(Math.abs(x2 - x1) < 1e-6)
            {
                if(x1 < left || x1 > right)
                    return false;
                const edgeMinY = Math.min(y1, y2);
                const edgeMaxY = Math.max(y1, y2);
                return !(edgeMaxY < top || edgeMinY > bottom);
            }

            const xa = Math.max(edgeMinX, left);
            const xb = Math.min(edgeMaxX, right);
            const ya = y1 + (xa - x1) * (y2 - y1) / (x2 - x1);
            const yb = y1 + (xb - x1) * (y2 - y1) / (x2 - x1);
            const segMinY = Math.min(ya, yb);
            const segMaxY = Math.max(ya, yb);
            return !(segMaxY < top || segMinY > bottom);
        };

        const snapY = function(v)
        {
            const g = main.grid_spacing || 1;
            return g * Math.round(v / g);
        };

        // Fourth pass: discrete local search minimizing "connection-over-component" events.
        for(let sweep = 0; sweep < 4; sweep++)
        {
            for(const layerIndex of layerIndices)
            {
                const layerNodes = layers.get(layerIndex);
                if(!layerNodes || layerNodes.length <= 1)
                    continue;

                layerNodes.sort((a, b) => a.y - b.y);
                for(let i = 0; i < layerNodes.length; i++)
                {
                    const n = layerNodes[i];
                    const prev = i > 0 ? layerNodes[i - 1] : null;
                    const next = i < layerNodes.length - 1 ? layerNodes[i + 1] : null;
                    const minYAllowed = prev ? (prev.y + prev.height + minGap) : -Infinity;
                    const maxYAllowed = next ? (next.y - n.height - minGap) : Infinity;

                    const neighbors = [...incoming.get(n.localName), ...outgoing.get(n.localName)]
                        .map((k) => nodeMap.get(k))
                        .filter((x) => !!x);
                    const targetY = neighbors.length
                        ? neighbors.reduce((s, m) => s + (m.y + m.height / 2), 0) / neighbors.length - n.height / 2
                        : n.y;

                    const originalY = n.y;
                    let bestY = originalY;
                    let bestScore = Infinity;
                    for(let step = -8; step <= 8; step++)
                    {
                        let candidateY = snapY(originalY + step * minGap);
                        if(candidateY < minYAllowed)
                            candidateY = minYAllowed;
                        if(candidateY > maxYAllowed)
                            candidateY = maxYAllowed;
                        n.y = candidateY;

                        let blockCount = 0;
                        for(const [u, v] of edges)
                            if(edgeCrossesNode(u, v, n))
                                blockCount++;

                        const distancePenalty = Math.abs(candidateY - targetY) / (minGap || 1);
                        const score = blockCount * 100 + distancePenalty;
                        if(score < bestScore)
                        {
                            bestScore = score;
                            bestY = candidateY;
                        }
                    }
                    n.y = bestY;
                }

                // Enforce ordering/non-overlap after local search.
                layerNodes.sort((a, b) => a.y - b.y);
                for(let i = 1; i < layerNodes.length; i++)
                {
                    const prev = layerNodes[i - 1];
                    const n = layerNodes[i];
                    const minYAllowed = prev.y + prev.height + minGap;
                    if(n.y < minYAllowed)
                        n.y = minYAllowed;
                }

                // Keep each layer centered while allowing extra vertical expansion when needed.
                const layerTop = Math.min(...layerNodes.map((n) => n.y));
                const layerBottom = Math.max(...layerNodes.map((n) => n.y + n.height));
                const offset = -((layerTop + layerBottom) / 2);
                for(const n of layerNodes)
                    n.y += offset;
            }
        }

        const totalOverlapCount = function()
        {
            let count = 0;
            for(const [u, v] of edges)
                for(const w of nodes)
                    if(edgeCrossesNode(u, v, w))
                        count++;
            return count;
        };

        const totalConnectionLength = function()
        {
            let length = 0;
            for(const [u, v] of edges)
            {
                const x1 = u.x + u.width / 2;
                const y1 = u.y + u.height / 2;
                const x2 = v.x + v.width / 2;
                const y2 = v.y + v.height / 2;
                const dx = x2 - x1;
                const dy = y2 - y1;
                length += Math.sqrt(dx*dx + dy*dy);
            }
            return length;
        };

        // Final pass: greedy global minimization of edge-over-component overlaps.
        for(let pass = 0; pass < 3; pass++)
        {
            let improved = false;
            for(const layerIndex of layerIndices)
            {
                const layerNodes = layers.get(layerIndex);
                if(!layerNodes || layerNodes.length === 0)
                    continue;

                layerNodes.sort((a, b) => a.y - b.y);
                for(let i = 0; i < layerNodes.length; i++)
                {
                    const n = layerNodes[i];
                    const prev = i > 0 ? layerNodes[i - 1] : null;
                    const next = i < layerNodes.length - 1 ? layerNodes[i + 1] : null;
                    const minYAllowed = prev ? (prev.y + prev.height + minGap) : -Infinity;
                    const maxYAllowed = next ? (next.y - n.height - minGap) : Infinity;

                    const originalY = n.y;
                    let bestY = originalY;
                    let bestOverlap = totalOverlapCount();
                    let bestLength = totalConnectionLength();

                    const candidates = [];
                    for(let s = -6; s <= 6; s++)
                    {
                        if(s === 0)
                            continue;
                        candidates.push(snapY(originalY + s * minGap));
                    }

                    for(const candidateRaw of candidates)
                    {
                        const candidateY = clamp(candidateRaw, minYAllowed, maxYAllowed);
                        n.y = candidateY;
                        const overlap = totalOverlapCount();
                        if(overlap > bestOverlap)
                            continue;

                        const length = totalConnectionLength();
                        if(
                            overlap < bestOverlap ||
                            (overlap === bestOverlap && length < bestLength)
                        )
                        {
                            bestOverlap = overlap;
                            bestLength = length;
                            bestY = candidateY;
                        }
                    }

                    n.y = bestY;
                    if(bestY !== originalY)
                        improved = true;
                }
            }
            if(!improved)
                break;
        }

        const minX = Math.min(...nodes.map((n) => n.x));
        const minY = Math.min(...nodes.map((n) => n.y));
        const grid = main.grid_spacing || 24;
        const dx = (1 * grid) - minX;
        const dy = (2 * grid) - minY;
        const snap = function(v)
        {
            const g = main.grid_spacing || 1;
            return g * Math.round(v / g);
        };

        for(const n of nodes)
        {
            n.object._x = snap(n.x + dx);
            n.object._y = snap(n.y + dy);
        }

        network.rebuildDict();
        nav.populate();
        if(options.selectArranged === false)
            selector.selectItems([], background, false, false, true);
        else
        {
            const selected = nodes
                .filter((n) => n.object && n.object._tag !== "widget")
                .map((n) => n.fullName);
            selector.selectItems(selected, background, false, false, true);
        }
        main.addConnections();
    },
    
    newModule(moduleClass = "Module")
    {
        const name = network.uniqueID("Untitled_");
        const x = main.normalizeNewComponentX(main.new_position_x);
        const m =
        {
            name:name,
            class:moduleClass,
            color:"",
            _tag:"module",
            _x:x,
            _y:main.new_position_y,
            inputs: [],
            outputs: [],
            parameters:[]
        };
        const full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].modules.push(m);
        network.dict[full_name]=m;
        if(moduleClass !== "Module" && network.classinfo && network.classinfo[moduleClass])
            network.changeModuleClass(full_name, moduleClass);
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
        nav.populate();
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
        main.addLockedMirrorComponent(full_name);
        return full_name;
    },

    newGroup() // FIXME: Move to network
    {
        const name = network.uniqueID("Group_");
        const x = main.normalizeNewComponentX(main.new_position_x);
        const m =
        {
            name:name,
            color:"",
            _tag:"group",
            _x:x,
            _y:main.new_position_y,
            inputs: [],
            outputs: [],
            parameters:[],
            modules: [],
            groups:[],
            widgets:[]
        };
        const full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].groups.push(m);
        network.dict[full_name]=m;
        main.new_position_x += 30;
        main.new_position_y += 30;
    
        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }

        nav.populate();
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
        main.addLockedMirrorComponent(full_name);
    },

    newInput() // FIXME: Move to network
    {
        const name = network.uniqueID("Input_");
        const x = main.normalizeNewComponentX(main.new_position_x);
        const m =
        {
            name:name,
            color:"",
            _tag:"input",
            _x:x,
            _y:main.new_position_y
        };
        let full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].inputs.push(m);
        network.dict[full_name]=m;
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
        main.addLockedMirrorComponent(full_name);
    },

    newOutput() // FIXME: Move to network
    {
        const name = network.uniqueID("Output_");
        const x = main.normalizeNewComponentX(main.new_position_x);
        const m =
        {
            name:name,
            color:"",
            //size:"1",
            _tag:"output",
            _x:x,
            _y:main.new_position_y
        };
        const full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].outputs.push(m);
        network.dict[full_name]=m;
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
        main.addLockedMirrorComponent(full_name);
    },

    newWidget()
    {
        const name = network.uniqueID("Widget_");
        const defaultWidgetSize = main.grid_spacing * 8 + 1;
        const x = main.normalizeNewComponentX(main.new_position_x);
        const w = {
            "_tag": "widget",
            "name": name,
            "title": name,
            "class": "bar-graph",
            "show_frame": true,
            _x:x,
            _y:main.new_position_y,
            width: defaultWidgetSize,
            height: defaultWidgetSize
        };
        const full_name = selector.selected_background+'.'+name;

        network.dict[selector.selected_background].widgets.push(w);
        network.dict[full_name]=w;
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
        main.addLockedMirrorComponent(full_name);
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
    },

    deleteGroup(g)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(g);
        this.deleteConnectionsForComponent(name, group);
        group.groups = group.groups.filter(function(g) { return g.name !== name; });
        selector.selectItems([], null);
    },

    deleteModule(m)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(m);
        this.deleteConnectionsForComponent(name, group);
        group.modules = group.modules.filter(function(m) { return m.name !== name; });
        selector.selectItems([], null);
    },

    deleteInput(i)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(i);
        this.deleteConnectionsForComponent(name, group);
        group.inputs = group.inputs.filter(function(i) { return i.name !== name; });
        selector.selectItems([], null);
    },

    deleteOutput(o)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(o);
        this.deleteConnectionsForComponent(name, group);
        group.outputs = group.outputs.filter(function(o) { return o.name !== name; });
        selector.selectItems([], null);
    },

    deleteWidget(w)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(w);
        this.deleteConnectionsForComponent(name, group);
        group.widgets = group.widgets.filter(function(w) { return w.name !== name; });
        selector.selectItems([], null);
    },

    connectionReferencesComponent(endpoint, componentName)
    {
        const localEndpoint = getStringUpToBracket(endpoint);
        return localEndpoint === componentName || localEndpoint.startsWith(componentName + ".");
    },

    deleteConnectionsForComponent(componentName, group)
    {
        if(!group || !group.connections)
            return;

        group.connections = group.connections.filter((connection) =>
            !this.connectionReferencesComponent(connection.source, componentName) &&
            !this.connectionReferencesComponent(connection.target, componentName)
        );
    },

    deleteConnection(c)
    {
        const resolved = main.resolveConnectionId(c);
        if(!resolved || !resolved.group || !Array.isArray(resolved.group.connections))
            return null;

        resolved.group.connections = resolved.group.connections.filter((connection) => connection !== resolved.connection);
        return resolved.path;
    },

    deleteComponent()
    {
        let changedBackground = selector.selected_background;
        if(selector.selected_connection != null)
        {
            changedBackground = this.deleteConnection(selector.selected_connection) || changedBackground;
        }
        else
            for(let c of selector.selected_foreground)
            {
                main.deleteLockedMirrorComponent(c);
                switch(network.dict[c]._tag)
                {
                    case 'module':
                        this.deleteModule(c);
                        break;
                    case 'group':
                        this.deleteGroup(c);
                        break;
                    case 'input':
                        this.deleteInput(c);
                        break; 
                    case 'output':
                        this.deleteOutput(c);
                        break; 
                    case 'widget':
                        this.deleteWidget(c);
                        break; 
                }
            }
        network.rebuildDict();
        nav.populate();
        selector.selectItems([], selector.selected_background, false, false, true);
        main.refreshMatchingBackgroundPanes(changedBackground, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
        main.refreshAllPaneGeometry();
        controller.setTainted(true, "deleteComponent");
    },

    isTextEditingTarget(target)
    {
        if(!target)
            return false;
        const tagName = target.tagName;
        return tagName === "INPUT" ||
            tagName === "TEXTAREA" ||
            target.isContentEditable;
    },

    isFormEditingTarget(target)
    {
        if(!target)
            return false;
        const tagName = target.tagName;
        return tagName === "INPUT" ||
            tagName === "TEXTAREA" ||
            tagName === "SELECT" ||
            target.isContentEditable;
    },

    getTextEditingTarget()
    {
        const activeElement = document.activeElement;
        if(main.isTextEditingTarget(activeElement))
            return activeElement;
        return null;
    },

    insertTextAtTarget(target, text)
    {
        if(!target)
            return false;

        target.focus();
        if(target.tagName === "INPUT" || target.tagName === "TEXTAREA")
        {
            const start = target.selectionStart ?? target.value.length;
            const end = target.selectionEnd ?? start;
            if(typeof target.setRangeText === "function")
            {
                target.setRangeText(text, start, end, "end");
                const inputEvent = typeof InputEvent === "function" ?
                    new InputEvent("input", {bubbles: true, inputType: "insertText", data: text}) :
                    new Event("input", {bubbles: true});
                target.dispatchEvent(inputEvent);
                return true;
            }
        }

        return document.execCommand("insertText", false, text);
    },

    async performTextClipboardCommand(action, target)
    {
        if(!target)
            return false;

        target.focus();
        if(action === "paste")
        {
            if(navigator.clipboard && typeof navigator.clipboard.readText === "function")
            {
                try
                {
                    const text = await navigator.clipboard.readText();
                    return main.insertTextAtTarget(target, text);
                }
                catch(error)
                {
                    console.log("clipboard paste failed", error);
                }
            }
            return document.execCommand("paste");
        }

        return document.execCommand(action);
    },

    handleClipboardMenuAction(action)
    {
        const target = main.getTextEditingTarget();
        if(target)
        {
            main.performTextClipboardCommand(action, target);
            return true;
        }
        return false;
    },

    uniqueComponentName(baseName, reservedNames=new Set())
    {
        const background = selector.selected_background;
        let index = 1;
        let candidate = `${baseName}${index}`;
        while((`${background}.${candidate}` in network.dict) || reservedNames.has(candidate))
            candidate = `${baseName}${++index}`;
        return candidate;
    },

    remapConnectionEndpoint(endpoint, nameMap)
    {
        const beforeBracket = getStringUpToBracket(endpoint);
        const afterBracket = getStringAfterBracket(endpoint);
        const parts = beforeBracket.split('.');
        if(parts.length > 0 && parts[0] in nameMap)
            parts[0] = nameMap[parts[0]];
        return parts.join('.') + afterBracket;
    },

    getSelectedComponentClipboardData()
    {
        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group || selector.selected_foreground.length === 0)
            return null;

        const selectedFullNames = [...selector.selected_foreground];
        const selectedLocalNames = new Set();
        const components = [];

        for(const fullName of selectedFullNames)
        {
            const component = network.dict[fullName];
            if(!component || !component.name || !component._tag)
                continue;
            selectedLocalNames.add(component.name);
            components.push(deepCopy(component));
        }

        if(components.length === 0)
            return null;

        const connections = [];
        for(const c of group.connections || [])
        {
            const sourceComponent = getStringUpToBracket(c.source || "").split('.')[0];
            const targetComponent = getStringUpToBracket(c.target || "").split('.')[0];
            if(selectedLocalNames.has(sourceComponent) && selectedLocalNames.has(targetComponent))
                connections.push(deepCopy(c));
        }

        return {
            type: "ikaros/components",
            components,
            connections
        };
    },

    copySelectedComponents()
    {
        if(!main.edit_mode)
            return false;

        const clipboardData = main.getSelectedComponentClipboardData();
        if(!clipboardData)
            return false;

        main.component_clipboard = clipboardData;
        return true;
    },

    cutSelectedComponents()
    {
        if(!main.copySelectedComponents())
            return false;
        main.deleteComponent();
        return true;
    },

    pasteComponents()
    {
        if(!main.edit_mode || !main.component_clipboard || main.component_clipboard.type !== "ikaros/components")
            return false;

        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group)
            return false;

        const reservedNames = new Set();
        const nameMap = {};
        const pastedFullNames = [];

        for(const original of main.component_clipboard.components || [])
        {
            if(!original || !original.name || !original._tag)
                continue;

            const oldName = original.name;
            const newName = main.uniqueComponentName(oldName, reservedNames);
            reservedNames.add(newName);
            nameMap[oldName] = newName;

            const clone = deepCopy(original);
            clone.name = newName;
            if(clone._x !== undefined)
                clone._x = parseInt(clone._x) + 30;
            if(clone._y !== undefined)
                clone._y = parseInt(clone._y) + 30;

            const collectionName = main.getComponentCollectionName(clone._tag);
            if(!collectionName)
                continue;
            if(!group[collectionName])
                group[collectionName] = [];
            group[collectionName].push(clone);
            pastedFullNames.push(`${background}.${newName}`);
        }

        if(pastedFullNames.length === 0)
            return false;

        if(!group.connections)
            group.connections = [];
        for(const c of main.component_clipboard.connections || [])
        {
            const cc = deepCopy(c);
            cc.source = main.remapConnectionEndpoint(c.source, nameMap);
            cc.target = main.remapConnectionEndpoint(c.target, nameMap);
            group.connections.push(cc);
        }

        network.rebuildDict();
        nav.populate();
        selector.selectItems(pastedFullNames, background, false, false, true);
        for(const fullName of pastedFullNames)
            main.addLockedMirrorComponent(fullName);
        main.refreshMatchingBackgroundPanes(background, pastedFullNames, main.active_pane ? main.active_pane.root : null);
        main.refreshAllPaneGeometry();
        controller.setTainted(true, "pasteComponents");
        return true;
    },

    duplicateSelectedComponents(include_external_connections=false)
    {
        if(!main.edit_mode)
            return;
        if(selector.selected_foreground.length === 0)
            return;

        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group)
            return;

        const selectedFullNames = [...selector.selected_foreground];
        const reservedNames = new Set();
        const nameMap = {};
        const duplicatedFullNames = [];

        for(const fullName of selectedFullNames)
        {
            const original = network.dict[fullName];
            if(!original || !original.name || !original._tag)
                continue;

            const oldName = original.name;
            const newName = main.uniqueComponentName(oldName, reservedNames);
            reservedNames.add(newName);
            nameMap[oldName] = newName;

            const clone = deepCopy(original);
            clone.name = newName;
            if(clone._x !== undefined)
                clone._x = parseInt(clone._x) + 30;
            if(clone._y !== undefined)
                clone._y = parseInt(clone._y) + 30;

            switch(clone._tag)
            {
                case 'group':
                    if(!group.groups) group.groups = [];
                    group.groups.push(clone);
                    break;
                case 'module':
                    if(!group.modules) group.modules = [];
                    group.modules.push(clone);
                    break;
                case 'input':
                    if(!group.inputs) group.inputs = [];
                    group.inputs.push(clone);
                    break;
                case 'output':
                    if(!group.outputs) group.outputs = [];
                    group.outputs.push(clone);
                    break;
                case 'widget':
                    if(!group.widgets) group.widgets = [];
                    group.widgets.push(clone);
                    break;
                default:
                    continue;
            }

            duplicatedFullNames.push(`${background}.${newName}`);
        }

        if(group.connections && Object.keys(nameMap).length > 0)
        {
            const duplicatedConnections = [];
            for(const c of group.connections)
            {
                const sourceComponent = getStringUpToBracket(c.source).split('.')[0];
                const targetComponent = getStringUpToBracket(c.target).split('.')[0];
                const sourceMapped = sourceComponent in nameMap;
                const targetMapped = targetComponent in nameMap;
                if(include_external_connections)
                {
                    if(!sourceMapped && !targetMapped)
                        continue;
                }
                else if(!sourceMapped || !targetMapped)
                    continue;

                const cc = deepCopy(c);
                cc.source = main.remapConnectionEndpoint(c.source, nameMap);
                cc.target = main.remapConnectionEndpoint(c.target, nameMap);
                duplicatedConnections.push(cc);
            }
            group.connections.push(...duplicatedConnections);
        }

        if(duplicatedFullNames.length === 0)
            return;

        network.rebuildDict();
        nav.populate();
        selector.selectItems(duplicatedFullNames, background, false, false, true);
        for(const fullName of duplicatedFullNames)
            main.addLockedMirrorComponent(fullName);
    },

    changeComponentPosition(c, dx,dy, snap_to_grid=true)
    {
        const e = main.getPaneElement(c);
        if(!e)
            return;

        const base = main.map[c] || [e.offsetLeft, e.offsetTop];
        let new_x = base[0] + dx;
        let new_y = base[1] + dy;
        const background = selector ? selector.selected_background : "";
        const mirrorSide = main.getMirrorSideForBackground(background);

        if(new_x < 0)
                new_x = 0;
        if(mirrorSide === "left")
            new_x = Math.min(new_x, main.getMirrorPaneWidth());

            if(new_y < 0)
                new_y = 0;

        if(main.edit_mode && snap_to_grid)
        {
            const g = main.grid_spacing;
            new_x = g*Math.round(new_x/g);
            new_y = g*Math.round(new_y/g);
        }

        e.style.left = new_x+"px";
        e.style.right = "auto";
        e.style.top = new_y+"px";

        network.dict[c]._x = mirrorSide === "right" ? new_x + e.offsetWidth : main.paneXToModelX(new_x, background);
        network.dict[c]._y = new_y;
        main.updateLockedMirrorComponentPosition(c);
    },

    changeComponentSize(dX, dY)
    {
        const w_id = selector.selected_foreground[0];
        const w = main.getPaneElement(w_id);
        if(!w)
            return;
        const newWidth = main.grid_spacing*Math.round((main.startX + dX)/main.grid_spacing)+1;
        const newHeight = main.grid_spacing*Math.round((main.startY + dY)/main.grid_spacing)+1;

        w.style.width = newWidth + 'px';
        w.style.height = newHeight + 'px';
        network.dict[w_id].width = newWidth;
        network.dict[w_id].height = newHeight;

        const widgetElement = main.getFrameWidget(w);
        if(widgetElement)
            widgetElement.parameterChangeNotification(network.dict[w_id]);
    },

    moveComponents(evt)
    {
        if(Object.keys(main.map).length == 0) // FIME: SHOULD NOT BE NEEDED
            return;

        evt.stopPropagation()
        const dx = evt.clientX - main.initialMouseX;
        const dy = evt.clientY - main.initialMouseY;

        for(let c of selector.selected_foreground)
            main.changeComponentPosition(c, dx,dy);
        main.addConnections();
        main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
    },

    releaseComponents(evt)
    {
        const main_view = main.view;
        main_view.removeEventListener('mousemove',main.moveComponents, true);
        main_view.removeEventListener('mouseup',main.releaseComponents, true);
        main_view.removeEventListener('mousemove',main.moveComponents, false);
        main_view.removeEventListener('mouseup',main.releaseComponents, false);
        for(const c of selector.selected_foreground)
        {
            const e = main.getPaneElement(c);
            if(e)
                e.classList.remove("dragged");
        }
        main.map = {};
        main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
    },

    releaseResizeComponent(evt)
    {
        const main_view = main.view;
        main_view.removeEventListener('mousemove',main.resizeComponent, true);
        main_view.removeEventListener('mouseup',main.releaseResizeComponent, true);
        main_view.removeEventListener('mousemove',main.resizeComponent, false);
        main.view.removeEventListener('mouseup',main.releaseResizeComponent, false);
        main.map = {}; // just in case

        const dX = evt.clientX - main.initialMouseX;
        const dY = evt.clientY - main.initialMouseY;
        main.changeComponentSize(dX,dY);

        const w_id = selector.selected_foreground[0];
        const w = main.getPaneElement(w_id);
        if(!w)
            return;
        const newWidth = main.grid_spacing*Math.round((main.startX + dX)/main.grid_spacing)+1;
        const newHeight = main.grid_spacing*Math.round((main.startY + dY)/main.grid_spacing)+1;

        w.style.width = newWidth + 'px';
        w.style.height = newHeight + 'px';
        network.dict[w_id].width = newWidth;
        network.dict[w_id].height = newHeight;
        w.classList.remove("resized");
        main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
    },

    startResize(evt)
    {
        evt.stopPropagation();
        main.startX = this.offsetLeft;
        main.startY = this.offsetTop;
        selector.selectItemsAndReveal([this.parentElement.dataset.name]);
        this.parentElement.classList.add("resized");

        main.initialMouseX = evt.clientX;
        main.initialMouseY = evt.clientY;
        main.view.addEventListener('mousemove',main.resizeComponent,true);
        main.view.addEventListener('mouseup',main.releaseResizeComponent,true);
        return false;
    },

    resizeComponent(evt) {
        const dX = evt.clientX - main.initialMouseX;
        const dY = evt.clientY - main.initialMouseY;
        main.changeComponentSize(dX,dY);
        return false;
    },

    startDragComponents(evt)
    {
        if(main.inline_name_edit)
            main.finishInlineNameEdit(false);
        evt.stopPropagation();
        if(evt.detail == 2) // handle double clicks elsewhere
        {
            const titleText = main.getEditableTitleTarget(evt.target);
            if(titleText)
            {
                const fullName = titleText.dataset.component || (this.dataset ? this.dataset.name : "");
                if(fullName)
                    setTimeout(function() { main.startInlineNameEditForComponent(fullName); }, 0);
            }
            evt.stopPropagation();
            return;
        }

       // this.style.pointerEvents = "none";

        main.initialMouseX = evt.clientX;
        main.initialMouseY = evt.clientY;

        const clickedName = this.dataset.name;
        const isAlreadySelected = selector.selected_foreground.includes(clickedName);
        if(evt.shiftKey || !isAlreadySelected)
            selector.selectItemsAndReveal([clickedName], null, evt.shiftKey);

        if(!selector.selected_foreground.includes(clickedName))
            return;

        if(!main.edit_mode)
            return;

        main.map = {};
        for(let c of selector.selected_foreground)
        {
            const e = main.getPaneElement(c);
            if(e)
                main.map[c] = [e.offsetLeft, e.offsetTop];
        }

        main.view.addEventListener('mousemove',main.moveComponents, true);
        main.view.addEventListener('mouseup',main.releaseComponents,true);

        return false;
    },

    startTrackConnection(evt)
    {
        if(!main.edit_mode)
            return;
        this.style.backgroundColor="orange";
        evt.stopPropagation();

        const id = main.getElementLogicalId(this);
        const e = main.getPaneElement(id);
        const viewRect = main.view.getBoundingClientRect();
        const elementRect = e.getBoundingClientRect();
        const x = elementRect.left-viewRect.left+4.5;
        const y = elementRect.top-viewRect.top+4.5;
        const clientX = elementRect.left + 4.5;
        const clientY = elementRect.top + 4.5;

        main.tracked_connection =  
        { 
            x1: x, 
            y1: y, 
            x2: x, 
            y2: y, 
            client_x1: clientX,
            client_y1: clientY,
            client_x2: clientX,
            client_y2: clientY,
            source: id.replace(/:out$/, ""),
            target: null, 
            source_element: e,
            target_element: null
        };
        document.addEventListener('mousemove',main.moveTrackedConnection, true);
        document.addEventListener('mouseup',main.releaseTrackedConnection,true);
    },

    clearTrackedConnectionHighlights(tracked)
    {
        if(!tracked)
            return;

        if(tracked.source_element)
            tracked.source_element.style.backgroundColor = "";

        if(tracked.target_element)
            tracked.target_element.style.backgroundColor = "";

        if(tracked.target)
        {
            const targetElement = main.getPaneElement(tracked.target);
            if(targetElement)
                targetElement.style.backgroundColor = "";
        }
    },

    moveTrackedConnection(evt)
    {
        if(!main.tracked_connection)
            return;

        const viewRect = main.view.getBoundingClientRect();
        const ox = viewRect.left;
        const oy = viewRect.top;
        main.tracked_connection.x2 = evt.clientX - ox;
        main.tracked_connection.y2 = evt.clientY - oy;
        main.tracked_connection.client_x2 = evt.clientX;
        main.tracked_connection.client_y2 = evt.clientY;
        main.addConnections();
    },

    getConnectionTargetIdFromElement(element)
    {
        if(!element)
            return null;

        if(element.matches && element.matches(".i_spot") && main.getElementLogicalId(element))
            return main.getElementLogicalId(element);

        if(element.matches && element.matches(".widget"))
        {
            if(main.getElementLogicalId(element))
                return main.getElementLogicalId(element);
            const frame = element.closest(".gi.widget");
            if(frame && main.getElementLogicalId(frame))
                return main.getElementLogicalId(frame);
        }

        const socket = element.closest ? element.closest(".i_spot") : null;
        if(socket && main.getElementLogicalId(socket))
            return main.getElementLogicalId(socket);

        const widgetFrame = element.closest ? element.closest(".gi.widget") : null;
        if(widgetFrame && main.getElementLogicalId(widgetFrame))
            return main.getElementLogicalId(widgetFrame);

        return null;
    },

    getEditableTitleTarget(target)
    {
        let element = target || null;
        if(element && element.nodeType === Node.TEXT_NODE)
            element = element.parentElement;
        if(!element || !element.closest)
            return null;
        return element.closest(".component-title-text");
    },

    releaseTrackedConnection(evt) 
    {
        evt.stopPropagation();
    
        if (!main.tracked_connection)
            return;

        const tracked = main.tracked_connection;
        const { source } = tracked;
        let target = null;

        const connectionsLayer = main.active_pane ? main.active_pane.connections_layer : null;
        let previousPointerEvents = null;
        if(connectionsLayer)
        {
            previousPointerEvents = connectionsLayer.style.pointerEvents;
            connectionsLayer.style.pointerEvents = "none";
        }

        if(document.elementsFromPoint)
        {
            const stack = document.elementsFromPoint(evt.clientX, evt.clientY) || [];
            for(const el of stack)
            {
                const resolved = main.getConnectionTargetIdFromElement(el);
                if(resolved)
                {
                    target = resolved;
                    break;
                }
            }
        }
        else
        {
            const hovered = document.elementFromPoint(evt.clientX, evt.clientY);
            target = main.getConnectionTargetIdFromElement(hovered);
        }

        if(connectionsLayer)
            connectionsLayer.style.pointerEvents = previousPointerEvents;

        if(!target)
            target = tracked.target || null;
    
        if (!target) 
        {
            main.clearTrackedConnectionHighlights(tracked);
            main.tracked_connection = null;
            main.addConnections();
            document.removeEventListener('mousemove',main.moveTrackedConnection, true);
            document.removeEventListener('mouseup',main.releaseTrackedConnection,true);
            return;
        }
    
        const isTargetWidget = target in network.dict && network.dict[target]._tag === 'widget';
        const connectionPath = main.getConnectionOwnerPath(source, target);
        let mirroredConnectionPath = "";
    
        if (isTargetWidget) 
        {
            const widgetParentPath = parentPath(target);
            const cleanSource = main.getRelativeConnectionEndpoint(source, widgetParentPath || connectionPath);
            const currentTitle = typeof network.dict[target].title === "string" ? network.dict[target].title : "";
            const currentSource = typeof network.dict[target].source === "string" ? network.dict[target].source : "";
            const shouldUpdateTitle = currentTitle.startsWith("Widget") || (currentTitle !== "" && currentTitle === currentSource);
            network.dict[target].source = cleanSource;
            if(shouldUpdateTitle)
                network.dict[target].title = cleanSource;

            const widgetFrame = main.getPaneElement(target);
            const widgetElement = main.getFrameWidget(widgetFrame);
            if(widgetElement)
            {
                widgetElement.parameters.source = cleanSource;
                if(shouldUpdateTitle)
                    widgetElement.parameters.title = cleanSource;
                try
                {
                    widgetElement.updateAll();
                }
                catch(err)
                {
                    console.log(err);
                }
            }
            main.clearTrackedConnectionHighlights(tracked);
            main.tracked_connection = null;
            selector.selectItems([target], null);
        } 
        else 
        {
            const cleanSource = main.getRelativeConnectionEndpoint(source, connectionPath);
            const cleanTarget = main.getRelativeConnectionEndpoint(target, connectionPath);
            network.newConnection(connectionPath, cleanSource, cleanTarget);
            const sourceConnection = network.dict[`${connectionPath}.${cleanSource}*${connectionPath}.${cleanTarget}`];
            mirroredConnectionPath = main.addMirroredConnectionForEndpoints(source, target, sourceConnection);
            main.clearTrackedConnectionHighlights(tracked);
            main.tracked_connection = null;
            selector.selectConnection(`${connectionPath}.${cleanSource}*${connectionPath}.${cleanTarget}`);
        }

        document.removeEventListener('mousemove',main.moveTrackedConnection, true);
        document.removeEventListener('mouseup',main.releaseTrackedConnection,true);
        main.addConnections();
        main.refreshMatchingBackgroundPanes(connectionPath, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
        if(mirroredConnectionPath && mirroredConnectionPath !== connectionPath)
            main.refreshMatchingBackgroundPanes(mirroredConnectionPath, [], main.active_pane ? main.active_pane.root : null);
        main.refreshAllPaneGeometry();
    },

    
    setConnectectionTarget(evt)
    {
        if(!main.tracked_connection)
            return;

        if(main.tracked_connection.target_element && main.tracked_connection.target_element !== this)
            main.tracked_connection.target_element.style.backgroundColor = "";

        this.style.backgroundColor="orange";
        const targetId = main.getConnectionTargetIdFromElement(this);
        main.tracked_connection.target_element = this;
        if(targetId)
            main.tracked_connection.target = targetId;
    },

    resetConnectectionTarget(evt)
    {
        if(!main.tracked_connection)
            return;
        main.tracked_connection.target = null;
        if(main.tracked_connection.target_element === this)
            main.tracked_connection.target_element = null;
        this.style.backgroundColor="";
    },

    getComponentColorPalette(component)
    {
        if(!component || typeof component.color !== "string")
            return null;

        const colorValue = component.color.trim();
        if(colorValue === "")
            return null;
        const colorName = colorValue.toLowerCase();

        const palettes = {
            red:    { bg:"#5e1a1a", titleBg:"#7a2020", rowBg:"#9a2b2b", classBg:"#8a2525", separator:"#a74747", titleFg:"#ffecec", rowFg:"#ffdede" },
            orange: { bg:"#8b4a1f", titleBg:"#a85a24", rowBg:"#cf7d3f", classBg:"#b8672d", separator:"#c98958", titleFg:"#fff2e8", rowFg:"#ffe6d2" },
            yellow: { bg:"#aa9445", titleBg:"#9b7f32", rowBg:"#fff79a", classBg:"#b28c39", separator:"#d8d08b", border:"#333333", titleFg:"#fff7d6", rowFg:"#2b2b2b" },
            green:  { bg:"#234426", titleBg:"#2d5a31", rowBg:"#397241", classBg:"#2a522e", separator:"#4f8d58", titleFg:"#e6f5e7", rowFg:"#d5ecd7" },
            blue:   { bg:"#233a5b", titleBg:"#2c4b73", rowBg:"#375f8f", classBg:"#294864", separator:"#4a78a6", titleFg:"#e8f0ff", rowFg:"#dbe8ff" },
            purple: { bg:"#3f2a55", titleBg:"#51356d", rowBg:"#66458a", classBg:"#4a3164", separator:"#7d58a6", titleFg:"#f1e9ff", rowFg:"#e5d8fb" },
            pink:   { bg:"#8c4f71", titleBg:"#a65f86", rowBg:"#e6a8ca", classBg:"#cb7faa", separator:"#b27398", titleFg:"#fff4fa", rowFg:"#ffe8f3" },
            white:  { bg:"#d8d8d8", titleBg:"#ebebeb", rowBg:"#f4f4f4", classBg:"#dfdfdf", separator:"#c3c3c3", border:"#333333", titleFg:"#222222", rowFg:"#333333" }
        };
        if(Object.prototype.hasOwnProperty.call(palettes, colorName))
            return palettes[colorName];
        if(/^(#)?[0-9a-fA-F]{6}$/.test(colorValue))
            return buildInspectorDerivedPalette(colorValue);
        return null;
    },

    getComponentStyleVars(component)
    {
        const p = main.getComponentColorPalette(component);
        if(!p)
            return "";
        const borderVar = p.border ? `--component-border:${p.border};` : "";
        return `--component-bg:${p.bg};--component-title-bg:${p.titleBg};--component-row-bg:${p.rowBg};--component-class-bg:${p.classBg};--component-separator:${p.separator};--component-title-fg:${p.titleFg};--component-row-fg:${p.rowFg};${borderVar}`;
    },

    getPositionedComponentStyle(component, background=null)
    {
        const vars = main.getComponentStyleVars(component);
        return `top:${component._y}px;${main.getPositionedXStyle(component._x, background)}${vars}`;
    },

    applyPositionedComponentStyle(element, component, background=null)
    {
        if(!element || !component)
            return;
        element.style.top = `${component._y}px`;
        const side = main.getMirrorSideForBackground(background || (selector ? selector.selected_background : ""));
        const x = main.modelXToPaneX(component._x, background);
        if(side === "right")
        {
            element.style.right = `calc(100% - ${x}px)`;
            element.style.left = "auto";
        }
        else
        {
            element.style.left = `${x}px`;
            element.style.right = "auto";
        }
    },

    applyComponentColorToElement(element, component)
    {
        if(!element)
            return;
        const p = main.getComponentColorPalette(component);
        const keys = [
            "--component-bg",
            "--component-title-bg",
            "--component-row-bg",
            "--component-class-bg",
            "--component-separator",
            "--component-border",
            "--component-title-fg",
            "--component-row-fg"
        ];
        if(!p)
        {
            for(const k of keys)
                element.style.removeProperty(k);
            return;
        }
        element.style.setProperty("--component-bg", p.bg);
        element.style.setProperty("--component-title-bg", p.titleBg);
        element.style.setProperty("--component-row-bg", p.rowBg);
        element.style.setProperty("--component-class-bg", p.classBg);
        element.style.setProperty("--component-separator", p.separator);
        if(p.border)
            element.style.setProperty("--component-border", p.border);
        else
            element.style.removeProperty("--component-border");
        element.style.setProperty("--component-title-fg", p.titleFg);
        element.style.setProperty("--component-row-fg", p.rowFg);
    },

    getConnectionColorValue(connection)
    {
        if(!connection || typeof connection.color !== "string")
            return "";
        const c = connection.color.trim().toLowerCase();
        if(c === "" || c === "black")
            return "";
        const map = {
            red: "#c43a3a",
            orange: "#d57c2f",
            yellow: "#b7a22d",
            green: "#3f8c49",
            blue: "#3f69b7",
            purple: "#7551ad",
            pink: "#c06aa0",
            white: "#efefef"
        };
        return map[c] || connection.color;
    },

    buildRoundedOrthogonalPath(points, radius=10)
    {
        if(!points || points.length < 2)
            return "";
        if(points.length === 2)
            return `M ${points[0].x} ${points[0].y} L ${points[1].x} ${points[1].y}`;

        const rBase = Math.max(1, radius);
        let d = `M ${points[0].x} ${points[0].y}`;
        for(let i = 1; i < points.length - 1; i++)
        {
            const prev = points[i - 1];
            const curr = points[i];
            const next = points[i + 1];

            const v1x = curr.x - prev.x;
            const v1y = curr.y - prev.y;
            const v2x = next.x - curr.x;
            const v2y = next.y - curr.y;

            const len1 = Math.abs(v1x) + Math.abs(v1y);
            const len2 = Math.abs(v2x) + Math.abs(v2y);
            if(len1 < 1 || len2 < 1)
                continue;

            const dir1x = v1x === 0 ? 0 : (v1x > 0 ? 1 : -1);
            const dir1y = v1y === 0 ? 0 : (v1y > 0 ? 1 : -1);
            const dir2x = v2x === 0 ? 0 : (v2x > 0 ? 1 : -1);
            const dir2y = v2y === 0 ? 0 : (v2y > 0 ? 1 : -1);

            const r = Math.min(rBase, Math.floor(len1 * 0.5), Math.floor(len2 * 0.5));
            if(r < 1)
            {
                d += ` L ${curr.x} ${curr.y}`;
                continue;
            }

            const inX = curr.x - dir1x * r;
            const inY = curr.y - dir1y * r;
            const outX = curr.x + dir2x * r;
            const outY = curr.y + dir2y * r;

            d += ` L ${inX} ${inY}`;
            d += ` Q ${curr.x} ${curr.y}, ${outX} ${outY}`;
        }

        const last = points[points.length - 1];
        d += ` L ${last.x} ${last.y}`;
        return d;
    },

    shouldHighlightAutoRoutedConnection(group, connection)
    {
        return !!(group && group.auto_routing && connection && connection.line_type === "auto_route");
    },

    // Main view rendering for groups, modules, inputs, outputs, and widgets.
    addGroup(g,path)
        {
        const fullName = `${path}.${g.name}`;
        const domId = main.getPaneDomId(fullName);
        const mirrorRight = main.isRightMirrorBackground(path);
        let s = "";
        s += `<div class='gi module group' style='${main.getPositionedComponentStyle(g, path)}' id='${domId}' data-logical-id='${fullName}' data-name='${fullName}'>`;
        s += `<table>`;
        s += `<tr><td class='title' colspan='3'><span class='component-title-text' data-component='${fullName}'>${g.name}</span></td></tr>`;

        for(let i of g.inputs || [])
        {
            const spotId = `${path}.${g.name}.${i.name}:in`;
            if(mirrorRight)
                s += `<tr><td></td><td>${i.name}</td><td class='input'><div class='i_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td></tr>`;
            else
                s += `<tr><td class='input'><div class='i_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td><td>${i.name}</td><td></td></tr>`;
        }

        for(let o of g.outputs || [])
        {
            const spotId = `${path}.${g.name}.${o.name}:out`;
            if(mirrorRight)
                s += `<tr><td class='output'><div class='o_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td><td>${o.name}</td><td></td></tr>`;
            else
                s += `<tr><td></td><td>${o.name}</td><td class='output'><div class='o_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td></tr>`;
        }

        s += `</table>`;
        s += `</div>`;
        main.view.innerHTML += s;
   },

    addInput(i,path)
    {
        const fullName = `${path}.${i.name}`;
        const spotId = `${fullName}:out`;
        if(main.isRightMirrorBackground(path))
            main.view.innerHTML += `<div class='gi group_input' id='${main.getPaneDomId(fullName)}' data-logical-id='${fullName}' data-name='${fullName}' style='${main.getPositionedComponentStyle(i, path)}'>
            <div class='o_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div>
            <span class='component-title-text' data-component='${fullName}'>${i.name}</span>
            </div>`;
        else
            main.view.innerHTML += `<div class='gi group_input' id='${main.getPaneDomId(fullName)}' data-logical-id='${fullName}' data-name='${fullName}' style='${main.getPositionedComponentStyle(i, path)}'>
            <span class='component-title-text' data-component='${fullName}'>${i.name}</span>
            <div class='o_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div>
            </div>`;
    },

    addOutput(o,path)
    {
        const fullName = `${path}.${o.name}`;
        const spotId = `${fullName}:in`;
        if(main.isRightMirrorBackground(path))
            main.view.innerHTML += `<div class='gi group_output' id='${main.getPaneDomId(fullName)}' data-logical-id='${fullName}' data-name='${fullName}' style='${main.getPositionedComponentStyle(o, path)}'><span class='component-title-text' data-component='${fullName}'>${o.name}</span><div class='i_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></div>`;
        else
            main.view.innerHTML += `<div class='gi group_output' id='${main.getPaneDomId(fullName)}' data-logical-id='${fullName}' data-name='${fullName}' style='${main.getPositionedComponentStyle(o, path)}'><div class='i_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div><span class='component-title-text' data-component='${fullName}'>${o.name}</span></div>`;
    },

    addModule(m,path)
    {
         const fullName = `${path}.${m.name}`;
         const mirrorRight = main.isRightMirrorBackground(path);
         let s = "";
         s += `<div class='gi module' style='${main.getPositionedComponentStyle(m, path)}' id='${main.getPaneDomId(fullName)}' data-logical-id='${fullName}' data-name='${fullName}'>`;
         s += `<table>`;
         s += `<tr><td class='title' colspan='3'><span class='component-title-text' data-component='${fullName}'>${m.name}</span><button type='button' class='module-title-menu-button' aria-label='Module menu' title='Module menu' data-component='${fullName}'>&#9776;</button></td></tr>`;

             s += `<tr><td  colspan='3' class='class_line'>${m.class}<button type='button' class='module-class-menu-button' aria-label='Class menu' title='Class menu' data-component='${fullName}'>&#9776;</button></td></tr>`;
  
        for(let i of m.inputs || [])
        {
            const spotId = `${fullName}.${i.name}:in`;
            if(mirrorRight)
                s += `<tr><td class='output'></td><td>${i.name}</td><td class='input'><div class='i_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td></tr>`;
            else
                s += `<tr><td class='input'><div class='i_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td ><td>${i.name}</td><td class='output'></td></tr>`;
        }
  
        for(let o of m.outputs || [])
        {
            const spotId = `${fullName}.${o.name}:out`;
            if(mirrorRight)
                s += `<tr><td class='output'><div class='o_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td><td>${o.name}</td><td class='output'></td></tr>`;
            else
                s += `<tr><td class='output'></td><td>${o.name}</td><td class='output'><div class='o_spot' id='${main.getPaneDomId(spotId)}' data-logical-id='${spotId}'></div></td></tr>`;
        }

         s += `</table>`;
         s += `</div>`;
        main.view.innerHTML += s;
    },

    addWidget(w, path) {
        const fullName = `${path}.${w.name}`;
        const newObject = document.createElement("div");
        newObject.setAttribute("class", "frame visible gi widget");
        newObject.setAttribute("id", main.getPaneDomId(fullName));
        newObject.setAttribute("data-logical-id", fullName);
        newObject.setAttribute("data-name", fullName);

        // Set style and position before the custom widget connects; several widgets
        // read their parent frame during initialization.
        newObject.style.top = `${w._y}px`;
        if(main.isRightMirrorBackground(path))
        {
            newObject.style.right = `calc(100% - ${main.modelXToPaneX(w._x, path)}px)`;
            newObject.style.left = "auto";
        }
        else
            newObject.style.left = `${main.modelXToPaneX(w._x, path)}px`;
        newObject.style.width = `${w.width || 200}px`;
        newObject.style.height = `${w.height || 200}px`;
    
        const newTitle = document.createElement("div");
        newTitle.setAttribute("class", "title");
        newTitle.setAttribute("data-component", fullName);
        newTitle.innerHTML = `<span class="component-title-text" data-component="${fullName}" data-field="title">${w.title || w.name}</span>`;
        const titleText = newTitle.querySelector(".component-title-text");
        const startWidgetTitleEdit = function(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
            setTimeout(function() { main.startInlineNameEditForComponent(fullName); }, 0);
        };
        newTitle.addEventListener("dblclick", startWidgetTitleEdit, false);
        if(titleText)
            titleText.addEventListener("dblclick", startWidgetTitleEdit, false);
        newObject.appendChild(newTitle);

        const menuButton = document.createElement("button");
        menuButton.type = "button";
        menuButton.className = "widget-title-menu-button";
        menuButton.setAttribute("aria-label", "Widget menu");
        menuButton.setAttribute("title", "Widget menu");
        menuButton.setAttribute("data-component", fullName);
        menuButton.innerHTML = "&#9776;";
        newObject.appendChild(menuButton);
    
        const index = main.view.querySelectorAll(".frame").length;
        main.view.appendChild(newObject);
        //newObject.addEventListener('mousedown', main.startDragComponents, false);
    
        const widgetClass = `webui-widget-${w.class}`;
        let constr = webui_widgets.constructors[widgetClass];
        let widgetElement;
    
        if (!constr) {
            console.log(`Internal Error: No constructor found for ${widgetClass}`);
            widgetElement = new webui_widgets.constructors['webui-widget-text'];
            widgetElement.element = newObject;
            widgetElement.parameters['text'] = `"${widgetClass}" not found. Is it included in index.html?`;
            widgetElement.parameters['_index_'] = index;
        } else {
            widgetElement = new webui_widgets.constructors[widgetClass];
            for (let k in widgetElement.parameters) {
                if (w[k] === undefined) {
                    w[k] = widgetElement.parameters[k];
                } else {
                    let tp = widgetElement.param_types[k];
                    w[k] = setType(w[k], tp);
                }
            }
            if(w.show_frame === undefined)
                w.show_frame = true;
            widgetElement.parameters = w;
            widgetElement.parameters['_index_'] = index;
        }
    
        widgetElement.setAttribute('class', 'widget');
        newObject.appendChild(widgetElement);
    
        // Add handle for resizing
        const handle = document.createElement("div");
        handle.setAttribute("class", "handle");
        handle.addEventListener('mousedown', main.startResize, false);
        newObject.appendChild(handle);
    
        try {
            widgetElement.updateAll();
        } catch (err) {
            console.log(err);
        }
    },

    addConnection(c,path, routedPoints=null)
    {
        const activePaneRoot = main.active_pane ? main.active_pane.root : null;
        const visibility = main.classifyConnectionVisibility(path, c, activePaneRoot ? [activePaneRoot] : null);
        if(visibility.samePanes.length === 0)
            return;

        const source = getStringUpToBracket(c.source);
        const target = getStringUpToBracket(c.target);
        const connectionId = `${path}.${source}*${path}.${target}`;
        const connectionDomId = main.getPaneDomId(connectionId);
        const source_point = main.getPaneElement(`${path}.${source}:out`);
        const target_point = main.getPaneElement(`${path}.${target}:in`);

        if(!source_point || !target_point)
            return;
        
        const ox = main.view.getBoundingClientRect().left;
        const oy = main.view.getBoundingClientRect().top;
        const x1 = source_point.getBoundingClientRect().left-ox+4.5;
        const y1 = source_point.getBoundingClientRect().top-oy+4.5;
        const x2 = target_point.getBoundingClientRect().left-ox+4.5;
        const y2 = target_point.getBoundingClientRect().top-oy+4.5;
        const connectionColor = main.getConnectionColorValue(c);
        const styleAttr = connectionColor ? ` style="--connection-color:${connectionColor};"` : "";
        const lineType = (c.line_type || "line").toLowerCase();
        let cc = "";
        if(Array.isArray(routedPoints) && routedPoints.length >= 2)
        {
            const d = main.buildRoundedOrthogonalPath(routedPoints, 10);
            cc = `<path d='${d}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${connectionDomId}" data-logical-id="${connectionId}" data-target='${target}' onclick='selector.selectConnectionAndReveal("${connectionId}")' ondblclick='selector.selectConnectionAndReveal("${connectionId}")'/>`;
        }
        else if(lineType === "orthogonal")
        {
            const mx = Math.round((x1 + x2) / 2);
            const points = `${x1},${y1} ${mx},${y1} ${mx},${y2} ${x2},${y2}`;
            cc = `<polyline points='${points}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${connectionDomId}" data-logical-id="${connectionId}" data-target='${target}' onclick='selector.selectConnectionAndReveal("${connectionId}")' ondblclick='selector.selectConnectionAndReveal("${connectionId}")'/>`;
        }
        else if(lineType === "orthagonal rounded" || lineType === "orthogonal rounded")
        {
            const mx = Math.round((x1 + x2) / 2);
            const points = [{x:x1,y:y1}, {x:mx,y:y1}, {x:mx,y:y2}, {x:x2,y:y2}];
            const d = main.buildRoundedOrthogonalPath(points, 10);
            cc = `<path d='${d}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${connectionDomId}" data-logical-id="${connectionId}" data-target='${target}' onclick='selector.selectConnectionAndReveal("${connectionId}")' ondblclick='selector.selectConnectionAndReveal("${connectionId}")'/>`;
        }
        else if(lineType === "spline")
        {
            const dx = Math.abs(x2 - x1);
            const bend = Math.max(24, Math.round(dx * 0.5));
            const c1x = x1 + bend;
            const c1y = y1;
            const c2x = x2 - bend;
            const c2y = y2;
            const d = `M ${x1} ${y1} C ${c1x} ${c1y}, ${c2x} ${c2y}, ${x2} ${y2}`;
            cc = `<path d='${d}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${connectionDomId}" data-logical-id="${connectionId}" data-target='${target}' onclick='selector.selectConnectionAndReveal("${connectionId}")' ondblclick='selector.selectConnectionAndReveal("${connectionId}")'/>`;
        }
        else
            cc = `<line x1='${x1}' y1='${y1}' x2='${x2}' y2='${y2}' class='connection_line'${styleAttr} data-source='${c.source}' id="${connectionDomId}" data-logical-id="${connectionId}" data-target='${target}' onclick='selector.selectConnectionAndReveal("${connectionId}")' ondblclick='selector.selectConnectionAndReveal("${connectionId}")'/>`;
        main.connections += cc;
    },

    addTrackedConnection()
    {
    },

    handleGroupDoubleClick(evt)
    {
        selector.selectItems([], evt.currentTarget.dataset.name);
    },

    // Connection debug geometry and routing helpers.
    collectComponentRectangles()
    {
        main.component_rectangles = [];
        if(!main.view)
            return main.component_rectangles;

        const viewRect = main.view.getBoundingClientRect();
        const margin = 15;
        for(const element of main.view.querySelectorAll(".gi"))
        {
            if(element.classList.contains("widget"))
                continue;
            const rect = element.getBoundingClientRect();
            const x = Math.round(rect.left - viewRect.left - margin);
            const y = Math.round(rect.top - viewRect.top - margin);
            const width = Math.round(rect.width + margin * 2);
            const height = Math.round(rect.height + margin * 2);
            main.component_rectangles.push({
                id: main.getElementLogicalId(element),
                x,
                y,
                width,
                height
            });
        }

        return main.component_rectangles;
    },

    addDebugRectangles()
    {
        const rectangles = main.collectComponentRectangles();
        for(const rect of rectangles)
        {
            main.connections += `<rect class='component-debug-rectangle' x='${rect.x}' y='${rect.y}' width='${rect.width}' height='${rect.height}' data-component='${rect.id}'/>`;
        }
    },

    getOutputOwnerId(outputSpot)
    {
        const logicalId = main.getElementLogicalId(outputSpot);
        if(!logicalId)
            return "";
        const outputId = logicalId.replace(/:out$/, "");
        if(main.getPaneElement(outputId))
            return outputId;
        return parentPath(outputId);
    },

    collectOutputDebugLines()
    {
        main.output_debug_lines = [];
        if(!main.view)
            return main.output_debug_lines;

        const viewRect = main.view.getBoundingClientRect();
        const viewWidth = main.view.clientWidth || Math.round(viewRect.width) || 3000;
        const rectangles = main.component_rectangles || [];

        for(const outputSpot of main.view.querySelectorAll(".o_spot"))
        {
            const spotRect = outputSpot.getBoundingClientRect();
            const x1 = Math.round(spotRect.left - viewRect.left + spotRect.width * 0.5);
            const y = Math.round(spotRect.top - viewRect.top + spotRect.height * 0.5);
            const ownerId = main.getOutputOwnerId(outputSpot);
            let x2 = viewWidth;

            for(const rect of rectangles)
            {
                if(rect.id === ownerId)
                    continue;
                const top = rect.y;
                const bottom = rect.y + rect.height;
                if(y < top || y > bottom)
                    continue;
                if(rect.x <= x1)
                    continue;
                x2 = Math.min(x2, rect.x);
            }

            main.output_debug_lines.push({
                id: main.getElementLogicalId(outputSpot),
                ownerId,
                x1,
                y1: y,
                x2,
                y2: y
            });
        }

        return main.output_debug_lines;
    },

    addOutputDebugLines()
    {
        const lines = main.collectOutputDebugLines();
        for(const line of lines)
        {
            main.connections += `<line class='output-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}' data-output='${line.id}' data-owner='${line.ownerId}'/>`;
        }
    },

    getInputOwnerId(inputSpot)
    {
        const logicalId = main.getElementLogicalId(inputSpot);
        if(!logicalId)
            return "";
        const inputId = logicalId.replace(/:in$/, "");
        if(main.getPaneElement(inputId))
            return inputId;
        return parentPath(inputId);
    },

    collectInputDebugLines()
    {
        main.input_debug_lines = [];
        if(!main.view)
            return main.input_debug_lines;

        const viewRect = main.view.getBoundingClientRect();
        const rectangles = main.component_rectangles || [];

        for(const inputSpot of main.view.querySelectorAll(".i_spot"))
        {
            const spotRect = inputSpot.getBoundingClientRect();
            const xEnd = Math.round(spotRect.left - viewRect.left + spotRect.width * 0.5);
            const y = Math.round(spotRect.top - viewRect.top + spotRect.height * 0.5);
            const ownerId = main.getInputOwnerId(inputSpot);
            let xStart = 0;

            for(const rect of rectangles)
            {
                if(rect.id === ownerId)
                    continue;
                const top = rect.y;
                const bottom = rect.y + rect.height;
                if(y < top || y > bottom)
                    continue;
                const rectRight = rect.x + rect.width;
                if(rectRight >= xEnd)
                    continue;
                xStart = Math.max(xStart, rectRight);
            }

            main.input_debug_lines.push({
                id: main.getElementLogicalId(inputSpot),
                ownerId,
                x1: xStart,
                y1: y,
                x2: xEnd,
                y2: y
            });
        }

        return main.input_debug_lines;
    },

    addInputDebugLines()
    {
        const lines = main.collectInputDebugLines();
        for(const line of lines)
        {
            main.connections += `<line class='input-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}' data-input='${line.id}' data-owner='${line.ownerId}'/>`;
        }
    },

    getOutputRoutePitch()
    {
        return 22;
    },

    subtractBlockedIntervals(width, blocked)
    {
        let segments = [{x1: 0, x2: width}];
        for(const interval of blocked)
        {
            const nextSegments = [];
            for(const segment of segments)
            {
                if(interval.x2 <= segment.x1 || interval.x1 >= segment.x2)
                {
                    nextSegments.push(segment);
                    continue;
                }
                if(interval.x1 > segment.x1)
                    nextSegments.push({x1: segment.x1, x2: interval.x1});
                if(interval.x2 < segment.x2)
                    nextSegments.push({x1: interval.x2, x2: segment.x2});
            }
            segments = nextSegments;
            if(segments.length === 0)
                break;
        }
        return segments.filter((segment) => segment.x2 - segment.x1 > 1);
    },

    collectHorizontalRoutingLines()
    {
        main.horizontal_routing_lines = [];
        if(!main.view)
            return main.horizontal_routing_lines;

        const viewRect = main.view.getBoundingClientRect();
        const viewWidth = main.view.clientWidth || Math.round(viewRect.width) || 3000;
        const viewHeight = main.view.clientHeight || Math.round(viewRect.height) || 3000;
        const pitch = main.getOutputRoutePitch();
        const reservedLines = [
            ...(main.output_debug_lines || []),
            ...(main.input_debug_lines || [])
        ].filter((line) =>
            line &&
            Number.isFinite(line.y1) &&
            Number.isFinite(line.x1) &&
            Number.isFinite(line.x2)
        );

        for(let y = Math.round(pitch); y < viewHeight; y += pitch)
        {
            const blocked = [];
            for(const rect of main.component_rectangles || [])
            {
                const top = rect.y;
                const bottom = rect.y + rect.height;
                if(y < top || y > bottom)
                    continue;
                blocked.push({x1: rect.x, x2: rect.x + rect.width});
            }
            for(const reservedLine of reservedLines)
            {
                if(Math.abs(y - reservedLine.y1) >= pitch)
                    continue;
                const x1 = Math.min(reservedLine.x1, reservedLine.x2);
                const x2 = Math.max(reservedLine.x1, reservedLine.x2);
                blocked.push({x1, x2});
            }
            blocked.sort((a, b) => a.x1 - b.x1);

            const segments = main.subtractBlockedIntervals(viewWidth, blocked);
            for(const segment of segments)
            {
                main.horizontal_routing_lines.push({
                    x1: Math.round(segment.x1),
                    y1: y,
                    x2: Math.round(segment.x2),
                    y2: y
                });
            }
        }

        return main.horizontal_routing_lines;
    },

    addHorizontalRoutingLines()
    {
        const lines = main.collectHorizontalRoutingLines();
        for(const line of lines)
        {
            main.connections += `<line class='routing-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}'/>`;
        }
    },

    collectVerticalRoutingLines()
    {
        main.vertical_routing_lines = [];
        if(!main.view)
            return main.vertical_routing_lines;

        const viewRect = main.view.getBoundingClientRect();
        const viewWidth = main.view.clientWidth || Math.round(viewRect.width) || 3000;
        const viewHeight = main.view.clientHeight || Math.round(viewRect.height) || 3000;
        const pitch = main.getOutputRoutePitch();
        const candidateXs = [];

        for(const rect of main.component_rectangles || [])
        {
            const left = rect.x;
            const right = rect.x + rect.width;
            for(let i = 0; i < 5; i++)
            {
                candidateXs.push(left - i * pitch, right + i * pitch);
            }
        }

        const uniqueXs = [];
        for(const rawX of candidateXs)
        {
            const x = Math.round(rawX);
            if(x < 0 || x > viewWidth)
                continue;
            if(uniqueXs.some((existingX) => Math.abs(existingX - x) < pitch))
                continue;
            uniqueXs.push(x);
        }

        for(const x of uniqueXs)
        {
            const blocked = [];
            for(const rect of main.component_rectangles || [])
            {
                const left = rect.x;
                const right = rect.x + rect.width;
                if(x < left || x > right)
                    continue;
                blocked.push({x1: rect.y, x2: rect.y + rect.height});
            }
            blocked.sort((a, b) => a.x1 - b.x1);

            const segments = main.subtractBlockedIntervals(viewHeight, blocked);
            for(const segment of segments)
            {
                main.vertical_routing_lines.push({
                    x1: x,
                    y1: Math.round(segment.x1),
                    x2: x,
                    y2: Math.round(segment.x2)
                });
            }
        }

        return main.vertical_routing_lines;
    },

    addVerticalRoutingLines()
    {
        const lines = main.collectVerticalRoutingLines();
        for(const line of lines)
        {
            main.connections += `<line class='vertical-routing-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}'/>`;
        }
    },

    collectRoutingGridPoints()
    {
        main.routing_grid_points = [];
        const horizontalLines = [
            ...(main.horizontal_routing_lines || []),
            ...(main.output_debug_lines || []),
            ...(main.input_debug_lines || [])
        ];
        const verticalLines = main.vertical_routing_lines || [];
        const seen = new Set();

        for(const h of horizontalLines)
        {
            const hx1 = Math.min(h.x1, h.x2);
            const hx2 = Math.max(h.x1, h.x2);
            const hy = h.y1;
            for(const v of verticalLines)
            {
                const vy1 = Math.min(v.y1, v.y2);
                const vy2 = Math.max(v.y1, v.y2);
                const vx = v.x1;
                if(vx < hx1 || vx > hx2)
                    continue;
                if(hy < vy1 || hy > vy2)
                    continue;

                const key = `${Math.round(vx)}:${Math.round(hy)}`;
                if(seen.has(key))
                    continue;
                seen.add(key);
                main.routing_grid_points.push({
                    x: Math.round(vx),
                    y: Math.round(hy)
                });
            }
        }

        return main.routing_grid_points;
    },

    addRoutingGridPoints()
    {
        const points = main.collectRoutingGridPoints();
        for(const point of points)
        {
            main.connections += `<circle class='routing-grid-point' cx='${point.x}' cy='${point.y}' r='4'/>`;
        }
    },

    getRoutingNodeId(x, y)
    {
        return `${Math.round(x)},${Math.round(y)}`;
    },

    getConnectionSocketIds(path, connection)
    {
        const source = getStringUpToBracket(connection.source || "");
        const target = getStringUpToBracket(connection.target || "");
        return {
            sourceSocketId: `${path}.${source}:out`,
            targetSocketId: `${path}.${target}:in`
        };
    },

    getRoutingHorizontalLines()
    {
        return [
            ...(main.output_debug_lines || []),
            ...(main.input_debug_lines || []),
            ...(main.horizontal_routing_lines || [])
        ].map((line) => ({
            x1: Math.min(line.x1, line.x2),
            y1: line.y1,
            x2: Math.max(line.x1, line.x2),
            y2: line.y2,
            id: line.id || ""
        }));
    },

    getRoutingVerticalLines()
    {
        return (main.vertical_routing_lines || []).map((line) => ({
            x1: line.x1,
            y1: Math.min(line.y1, line.y2),
            x2: line.x2,
            y2: Math.max(line.y1, line.y2)
        }));
    },

    buildRoutingGraph()
    {
        const graph = {
            nodes: new Map(),
            edges: new Map(),
            adjacency: new Map()
        };
        const horizontals = main.getRoutingHorizontalLines();
        const verticals = main.getRoutingVerticalLines();

        const ensureNode = function(x, y)
        {
            const id = main.getRoutingNodeId(x, y);
            if(!graph.nodes.has(id))
                graph.nodes.set(id, {id, x: Math.round(x), y: Math.round(y)});
            if(!graph.adjacency.has(id))
                graph.adjacency.set(id, []);
            return id;
        };

        const addEdge = function(nodeA, nodeB, dir)
        {
            if(nodeA === nodeB)
                return;
            const key = nodeA < nodeB ? `${nodeA}|${nodeB}` : `${nodeB}|${nodeA}`;
            if(graph.edges.has(key))
                return;
            const a = graph.nodes.get(nodeA);
            const b = graph.nodes.get(nodeB);
            const length = dir === "H" ? Math.abs(a.x - b.x) : Math.abs(a.y - b.y);
            const edge = {key, a: nodeA, b: nodeB, dir, length};
            graph.edges.set(key, edge);
            graph.adjacency.get(nodeA).push(edge);
            graph.adjacency.get(nodeB).push(edge);
        };

        for(const h of horizontals)
        {
            const xs = [h.x1, h.x2];
            for(const other of horizontals)
            {
                if(other === h || Math.abs(other.y1 - h.y1) > 0.5)
                    continue;
                if(other.x1 > h.x2 || other.x2 < h.x1)
                    continue;
                xs.push(Math.max(h.x1, other.x1), Math.min(h.x2, other.x2), other.x1, other.x2);
            }
            for(const v of verticals)
            {
                if(v.x1 < h.x1 || v.x1 > h.x2)
                    continue;
                if(h.y1 < v.y1 || h.y1 > v.y2)
                    continue;
                xs.push(v.x1);
            }
            const uniqueXs = Array.from(new Set(xs.map((x) => Math.round(x)))).sort((a, b) => a - b);
            for(let i = 0; i < uniqueXs.length; i++)
                ensureNode(uniqueXs[i], h.y1);
            for(let i = 1; i < uniqueXs.length; i++)
                addEdge(main.getRoutingNodeId(uniqueXs[i - 1], h.y1), main.getRoutingNodeId(uniqueXs[i], h.y1), "H");
        }

        for(const v of verticals)
        {
            const ys = [v.y1, v.y2];
            for(const h of horizontals)
            {
                if(v.x1 < h.x1 || v.x1 > h.x2)
                    continue;
                if(h.y1 < v.y1 || h.y1 > v.y2)
                    continue;
                ys.push(h.y1);
            }
            const uniqueYs = Array.from(new Set(ys.map((y) => Math.round(y)))).sort((a, b) => a - b);
            for(let i = 0; i < uniqueYs.length; i++)
                ensureNode(v.x1, uniqueYs[i]);
            for(let i = 1; i < uniqueYs.length; i++)
                addEdge(main.getRoutingNodeId(v.x1, uniqueYs[i - 1]), main.getRoutingNodeId(v.x1, uniqueYs[i]), "V");
        }

        return graph;
    },

    compareRouteCost(a, b)
    {
        if(a.length !== b.length)
            return a.length - b.length;
        return a.segments - b.segments;
    },

    isRoutingEdgeAvailable(edgeKey, sourceSocketId, targetSocketId, occupiedEdges)
    {
        const occupants = occupiedEdges.get(edgeKey);
        if(!occupants || occupants.length === 0)
            return true;
        return occupants.every((occupant) => occupant.sourceSocketId === sourceSocketId || occupant.targetSocketId === targetSocketId);
    },

    findOrthogonalRoute(graph, sourceNodeId, targetNodeId, sourceSocketId, targetSocketId, occupiedEdges)
    {
        if(!graph.nodes.has(sourceNodeId) || !graph.nodes.has(targetNodeId))
            return null;

        const queue = [{
            nodeId: sourceNodeId,
            prevDir: null,
            length: 0,
            segments: 0,
            prevNodeId: null,
            prevState: null,
            viaEdgeKey: null
        }];
        const best = new Map();

        while(queue.length > 0)
        {
            let bestIndex = 0;
            for(let i = 1; i < queue.length; i++)
            {
                if(main.compareRouteCost(queue[i], queue[bestIndex]) < 0)
                    bestIndex = i;
            }
            const state = queue.splice(bestIndex, 1)[0];
            const bestKey = `${state.nodeId}|${state.prevDir || "-"}|${state.segments}`;
            const recorded = best.get(bestKey);
            if(recorded && main.compareRouteCost(recorded, state) < 0)
                continue;

            if(state.nodeId === targetNodeId && state.segments <= 5)
                return state;

            const currentNode = graph.nodes.get(state.nodeId);
            for(const edge of graph.adjacency.get(state.nodeId) || [])
            {
                if(!main.isRoutingEdgeAvailable(edge.key, sourceSocketId, targetSocketId, occupiedEdges))
                    continue;

                const nextNodeId = edge.a === state.nodeId ? edge.b : edge.a;
                if(state.prevNodeId && nextNodeId === state.prevNodeId)
                    continue;

                const nextNode = graph.nodes.get(nextNodeId);
                if(state.prevDir === null)
                {
                    if(edge.dir !== "H" || nextNode.x <= currentNode.x)
                        continue;
                }

                if(nextNodeId === targetNodeId)
                {
                    if(edge.dir !== "H" || currentNode.x >= nextNode.x)
                        continue;
                }

                const nextSegments = state.prevDir === null ? 1 : (state.prevDir === edge.dir ? state.segments : state.segments + 1);
                if(nextSegments > 5)
                    continue;

                const nextState = {
                    nodeId: nextNodeId,
                    prevDir: edge.dir,
                    length: state.length + edge.length,
                    segments: nextSegments,
                    prevNodeId: state.nodeId,
                    prevState: state,
                    viaEdgeKey: edge.key
                };
                const nextKey = `${nextState.nodeId}|${nextState.prevDir}|${nextState.segments}`;
                const bestNext = best.get(nextKey);
                if(bestNext && main.compareRouteCost(bestNext, nextState) <= 0)
                    continue;
                best.set(nextKey, nextState);
                queue.push(nextState);
            }
        }

        return null;
    },

    reconstructRoutePoints(graph, finalState)
    {
        const nodes = [];
        const edgeKeys = [];
        let state = finalState;
        while(state)
        {
            nodes.push(graph.nodes.get(state.nodeId));
            if(state.viaEdgeKey)
                edgeKeys.push(state.viaEdgeKey);
            state = state.prevState;
        }
        nodes.reverse();
        edgeKeys.reverse();

        const points = [];
        for(const node of nodes)
        {
            if(points.length < 2)
            {
                points.push({x: node.x, y: node.y});
                continue;
            }
            const prev = points[points.length - 1];
            const prevPrev = points[points.length - 2];
            const sameX = prevPrev.x === prev.x && prev.x === node.x;
            const sameY = prevPrev.y === prev.y && prev.y === node.y;
            if(sameX || sameY)
            {
                points[points.length - 1] = {x: node.x, y: node.y};
                continue;
            }
            points.push({x: node.x, y: node.y});
        }

        return {points, edgeKeys};
    },

    registerRouteOccupancy(edgeKeys, sourceSocketId, targetSocketId, occupiedEdges)
    {
        for(const edgeKey of edgeKeys)
        {
            if(!occupiedEdges.has(edgeKey))
                occupiedEdges.set(edgeKey, []);
            occupiedEdges.get(edgeKey).push({sourceSocketId, targetSocketId});
        }
    },

    buildAutoRoutesForGroup(group, path)
    {
        const routes = new Map();
        if(!group)
            return routes;

        const graph = main.buildRoutingGraph();
        const occupiedEdges = new Map();
        const sourceLineMap = new Map((main.output_debug_lines || []).map((line) => [line.id, line]));
        const targetLineMap = new Map((main.input_debug_lines || []).map((line) => [line.id, line]));

        const candidates = [];
        for(const connection of group.connections || [])
        {
            if(!main.shouldHighlightAutoRoutedConnection(group, connection))
                continue;
            const {sourceSocketId, targetSocketId} = main.getConnectionSocketIds(path, connection);
            const sourceLine = sourceLineMap.get(sourceSocketId);
            const targetLine = targetLineMap.get(targetSocketId);
            if(!sourceLine || !targetLine)
                continue;
            candidates.push({
                connection,
                sourceSocketId,
                targetSocketId,
                sourceLine,
                targetLine,
                estimatedLength: Math.abs(targetLine.x2 - sourceLine.x1) + Math.abs(targetLine.y1 - sourceLine.y1)
            });
        }

        candidates.sort((a, b) => a.estimatedLength - b.estimatedLength);

        for(const candidate of candidates)
        {
            const sourceNodeId = main.getRoutingNodeId(candidate.sourceLine.x1, candidate.sourceLine.y1);
            const targetNodeId = main.getRoutingNodeId(candidate.targetLine.x2, candidate.targetLine.y2);
            const finalState = main.findOrthogonalRoute(graph, sourceNodeId, targetNodeId, candidate.sourceSocketId, candidate.targetSocketId, occupiedEdges);
            if(!finalState)
                continue;

            const route = main.reconstructRoutePoints(graph, finalState);
            routes.set(main.getConnectionKey(path, candidate.connection), route.points);
            main.registerRouteOccupancy(route.edgeKeys, candidate.sourceSocketId, candidate.targetSocketId, occupiedEdges);
        }

        return routes;
    },

    getConnectionKey(path, connection)
    {
        const source = getStringUpToBracket(connection.source || "");
        const target = getStringUpToBracket(connection.target || "");
        return `${path}.${source}*${path}.${target}`;
    },

    // Inline editing for component names and widget titles.
    commitInlineRename(fullName, newName)
    {
        const item = network.dict[fullName];
        if(!item || !newName)
            return;

        const trimmedName = newName.trim();
        if(!trimmedName || trimmedName === item.name)
            return;

        const group = selector.selected_background;
        const newFullName = `${group}.${trimmedName}`;
        if(newFullName !== fullName && network.dict[newFullName])
            return;

        main.renameLockedMirrorComponent(fullName, trimmedName);
        item.name = trimmedName;
        if(item._tag == "input")
        {
            network.renameInput(group, fullName, newFullName);
            selector.selectItems([newFullName], null);
        }
        else if(item._tag == "output")
        {
            network.renameOutput(group, fullName, newFullName);
            selector.selectItems([newFullName], null);
        }
        else if(item._tag == "group" || item._tag == "module")
        {
            network.renameGroupOrModule(group, fullName, newFullName);
            selector.selectItems([newFullName], null);
        }
        else if(item._tag == "widget")
        {
            network.dict[newFullName] = item;
            delete network.dict[fullName];
            selector.selectItems([newFullName], null);
        }
        network.rebuildDict();
        nav.populate();
        controller.setTainted(true, "commitInlineNameEdit");
    },

    commitInlineTitleEdit(fullName, newTitle)
    {
        const item = network.dict[fullName];
        if(!item || item._tag !== "widget" || !newTitle)
            return;

        const trimmedTitle = newTitle.trim();
        if(!trimmedTitle || trimmedTitle === item.title)
            return;

        item.title = trimmedTitle;
        const widgetFrame = main.getPaneElement(fullName);
        const widgetElement = main.getFrameWidget(widgetFrame);
        if(widgetElement)
        {
            widgetElement.parameters.title = trimmedTitle;
            try
            {
                widgetElement.updateAll();
            }
            catch(err)
            {
                console.log(err);
            }
        }
        if(inspector && typeof inspector.showInspectorForSelection === "function")
            inspector.showInspectorForSelection();
        main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
        main.syncLockedMirrorItemProperties(fullName, ["title"]);
        controller.setTainted(true, "commitInlineTitleEdit");
    },

    beginRenameAfterSelection(fullName)
    {
        if(!fullName)
            return;
        setTimeout(function()
        {
            main.startInlineNameEditForComponent(fullName);
        }, 0);
    },

    finishInlineNameEdit(commit)
    {
        const session = main.inline_name_edit;
        if(!session || session.finished)
            return;

        session.finished = true;
        const title = session.title;
        const componentElement = session.componentElement;
        title.contentEditable = "false";
        title.classList.remove("inline-name-edit");
        title.removeEventListener("keydown", session.onKeyDown, true);
        title.removeEventListener("blur", session.onBlur, true);

        const candidateName = title.textContent.replace(/\n/g, "").trim();
        if(commit && candidateName)
        {
            if(session.field === "title")
                main.commitInlineTitleEdit(session.fullName, candidateName);
            else
                main.commitInlineRename(session.fullName, candidateName);
        }
        else
            title.textContent = session.originalName;

        if(componentElement)
            componentElement.style.minWidth = "";
        title.removeAttribute("tabindex");

        main.inline_name_edit = null;
    },

    startInlineNameEdit(evt)
    {
        if(!main.edit_mode)
            return;

        const title = evt.currentTarget || evt.target.closest(".component-title-text");
        if(!title)
            return;
        const componentElement = title.closest(".gi");
        const fullName = title.dataset.component || (componentElement ? componentElement.dataset.name : "");
        const item = fullName ? network.dict[fullName] : null;
        const field = title.dataset.field || "name";
        if(!item || !["group", "module", "input", "output", "widget"].includes(item._tag))
            return;
        if(item._tag === "widget" && field !== "title")
            return;

        evt.preventDefault();
        evt.stopPropagation();

        if(main.inline_name_edit)
            main.finishInlineNameEdit(false);

        if(selector.selected_foreground[0] !== fullName || selector.selected_foreground.length !== 1)
            selector.selectItems([fullName], selector.selected_background);

        const originalName = field === "title" ? (item.title || title.textContent || "") : (item.name || title.textContent || "");
        if(componentElement)
            componentElement.style.minWidth = `${Math.ceil(componentElement.getBoundingClientRect().width)}px`;
        title.contentEditable = "true";
        title.setAttribute("contenteditable", "true");
        title.tabIndex = -1;
        title.spellcheck = false;
        title.classList.add("inline-name-edit");
        title.textContent = originalName;
        title.focus({preventScroll:true});
        document.getSelection()?.selectAllChildren(title);

        const onKeyDown = function(keyEvt)
        {
            if(keyEvt.key === "Enter")
            {
                keyEvt.preventDefault();
                main.finishInlineNameEdit(true);
                title.blur();
                return;
            }
            if(keyEvt.key === "Escape")
            {
                keyEvt.preventDefault();
                main.finishInlineNameEdit(false);
                title.blur();
            }
        };

        const onBlur = function()
        {
            main.finishInlineNameEdit(false);
        };

        main.inline_name_edit = {
            title,
            componentElement,
            fullName,
            field,
            originalName,
            onKeyDown,
            onBlur,
            finished: false
        };
        title.addEventListener("keydown", onKeyDown, true);
        title.addEventListener("blur", onBlur, true);
    },

    getInlineEditFieldForComponent(fullName)
    {
        const item = fullName ? network.dict[fullName] : null;
        return item && item._tag === "widget" ? "title" : "name";
    },

    getInlineEditTitleElement(fullName, field=null)
    {
        if(!fullName)
            return null;
        if(!main.view || typeof CSS === "undefined" || typeof CSS.escape !== "function")
            return null;
        const escapedName = CSS.escape(fullName);
        const resolvedField = field || main.getInlineEditFieldForComponent(fullName);
        if(resolvedField === "title")
            return main.view.querySelector(`.component-title-text[data-component="${escapedName}"][data-field="title"]`);
        return main.view.querySelector(`.component-title-text[data-component="${escapedName}"]:not([data-field]), .component-title-text[data-component="${escapedName}"][data-field="name"]`);
    },

    startInlineNameEditForComponent(fullName, field=null)
    {
        const title = main.getInlineEditTitleElement(fullName, field);
        if(!title)
            return;
        const syntheticEvent = {
            currentTarget: title,
            preventDefault() {},
            stopPropagation() {}
        };
        main.startInlineNameEdit(syntheticEvent);
    },

    // Rendered connection selection and refresh.
    selectConnection(connection)
    {
        const resolved = main.resolveConnectionId(connection);
        if(!resolved)
            return;

        for(const c of main.getElementsByLogicalId(connection))
        {
            c.classList.add("selected");
        }

        for(const s of main.getElementsByLogicalId(resolved.sourceEndpointId))
            s.style.backgroundColor = "orange";
        for(const t of main.getElementsByLogicalId(resolved.targetEndpointId))
            t.style.backgroundColor = "orange";
    },

    deselectConnection(connection)
    {
        const resolved = main.resolveConnectionId(connection);
        for(const c of main.getElementsByLogicalId(connection))
        {
            c.classList.remove("selected");
        }

        if(!resolved)
            return;

        for(const s of main.getElementsByLogicalId(resolved.sourceEndpointId))
            s.style.backgroundColor = "rgb(177, 177, 177)";
        for(const t of main.getElementsByLogicalId(resolved.targetEndpointId))
            t.style.backgroundColor = "rgb(177, 177, 177)";
    },

    addConnections()
    {
        const path = main.active_pane && main.active_pane.root && main.getPaneBackground(main.active_pane.root) ? main.getPaneBackground(main.active_pane.root) : selector.selected_background;
        const group = network.dict[path];
        const s = main.active_pane ? main.active_pane.connections_layer : null;
        if(!s || !group)
            return;

        main.connections = "<svg xmlns='http://www.w3.org/2000/svg' class='connections_svg'>";
        main.collectComponentRectangles();
        main.collectOutputDebugLines();
        main.collectInputDebugLines();
        main.collectHorizontalRoutingLines();
        main.collectVerticalRoutingLines();
        main.collectRoutingGridPoints();
        const autoRoutes = main.buildAutoRoutesForGroup(group, path);
        for(let c of group.connections || [])
            main.addConnection(c, path, autoRoutes.get(main.getConnectionKey(path, c)) || null);
        main.addTrackedConnection();
        main.connections += "</svg>";
        s.innerHTML = main.connections;
        main.updateWorkspaceConnections();
    },

    updateComponentStates()
    {
    // Add object handlers for all visible elements in main view depending on mode and tyep

    const currentPane = main.getCurrentPaneRoot();
    // Duplicate panes show live edits, but selection is painted only in the active pane.
    const selectionList = main.isActivePane(currentPane) && Array.isArray(selector.selected_foreground) ? selector.selected_foreground : [];

        if(main.edit_mode)
        {
            for (let e of main.view.querySelectorAll(".gi")) 
            {
                // Main drag function
                e.addEventListener('mousedown', main.startDragComponents, false);
            
                // Double click behaviors
                if (e.classList.contains("group")) 
                {
                    e.removeEventListener('dblclick', main.handleGroupDoubleClick, false);
                    e.addEventListener('dblclick', main.handleGroupDoubleClick, false); // Jump into group
                }
                else 
                    e.removeEventListener('dblclick', main.handleGroupDoubleClick, false);
     
                // Set selection class if selected
                if(selectionList.includes(e.dataset.name))
                    e.classList.add("selected")
                else
                    e.classList.remove("selected"); // FIXME: is this ever necessary?
            }

            // Add handlerer to outputs
            for(let o of main.view.querySelectorAll(".o_spot"))
                o.addEventListener('mousedown', main.startTrackConnection, false);
    
            // Add handlers to inputs

            for(let i of main.view.querySelectorAll(".i_spot"))
            {
                i.addEventListener('mouseover', main.setConnectectionTarget, true);
                i.addEventListener('mouseleave', main.resetConnectectionTarget, true);
            }
    
            // Add handlers to widgets

            for(let i of main.view.querySelectorAll(".widget"))
            {
                i.addEventListener('mouseover', main.setConnectectionTarget, true);
                i.addEventListener('mouseleave', main.resetConnectectionTarget, true);
            }

            for(const b of main.view.querySelectorAll(".module-title-menu-button"))
                b.addEventListener('mousedown', main.openComponentColorMenuFromButton, false);
            for(const b of main.view.querySelectorAll(".module-class-menu-button"))
                b.addEventListener('mousedown', main.openComponentClassMenuFromButton, false);
            for(const b of main.view.querySelectorAll(".widget-title-menu-button"))
                b.addEventListener('mousedown', main.openWidgetMenuFromButton, false);
            for(const t of main.view.querySelectorAll(".component-title-text"))
                t.addEventListener('dblclick', main.startInlineNameEdit, false);
        }
        else // View mode
        {
            for (let e of main.view.querySelectorAll(".gi"))
            {
                if(!e.classList.contains("widget"))
                {
                    // Main drag function
                    e.addEventListener('mousedown', main.startDragComponents, false);   // FIXME: Change to select only hete
                
                    // Double click behaviors
                    if (e.classList.contains("group")) 
                    {
                        e.removeEventListener('dblclick', main.handleGroupDoubleClick, false);
                        e.addEventListener('dblclick', main.handleGroupDoubleClick, false); // Jump into group
                    }
                    else 
                        e.removeEventListener('dblclick', main.handleGroupDoubleClick, false);

            // Add handlerer to outputs
            for(let o of main.view.querySelectorAll(".o_spot"))
                    o.addEventListener('dblclick', function (evt) 
                        {

                            window.open("http://localhost:8000/data/"+main.getElementLogicalId(this).replace(/:out$/, ''), "_blank", "width=800,height=600");
                        }, false);

                    // Set selection class if selected
                }

                if(selectionList.includes(e.dataset.name))
                    e.classList.add("selected")
                else
                    e.classList.remove("selected"); // FIXME: is this ever necessary?
                }
            for(const t of main.view.querySelectorAll(".component-title-text"))
                t.addEventListener('dblclick', main.startInlineNameEdit, false);
        }
        if(main.isActivePane(currentPane))
            main.clearSelectionsOutsidePane(currentPane);
    },

    addComponents(group, selectionList, path)
    {
        if(group === undefined)
            return;

        main.view.innerHTML = "";

        for(let i of group.inputs || [])
            main.addInput(i,path);

        for(let o of group.outputs || [])
            main.addOutput(o,path);

        for(let g of group.groups || [])
            main.addGroup(g,path);

        for(let m of group.modules || [])
            main.addModule(m,path);

        for(let w of group.widgets || [])
            main.addWidget(w,path);

        main.addConnections();
        main.updateComponentStates();
    },

    renderIntoPane(pane, background, foreground=[])
    {
        if(!pane || !background)
            return;
        main.setPaneBackground(pane, background);
        main.withPane(pane, () =>
        {
            main.drawGrid();
            breadcrumbs.selectItem(background, pane);
            main.addComponents(network.dict[background], foreground, background);
        });
    },

    refreshMatchingBackgroundPanes(background, foreground=[], excludePane=null)
    {
        if(!background)
            return;
        main.getPanesForBackground(background, excludePane).forEach((pane) => main.renderIntoPane(pane, background, foreground));
    },

    // Selection rendering and edit/view mode transitions.
    cancelEditMode()
    {
        this.setViewMode();
    },

    setEditMode()
    {
        controller.setTainted(true, "setEditMode");
        main.main.classList.add("edit_mode");
        main.main.classList.remove("view_mode");
        main.edit_mode = true;
        controller.run_mode = 'stop';
        controller.get("stop", controller.update);
        main.updateComponentStates();
        inspector.updateLibraryAddButtonState();
        inspector.showInspectorForSelection();
    },

    setViewMode()
    {
        main.main.classList.add("view_mode");
        main.main.classList.remove("edit_mode");
        main.edit_mode = false;
        inspector.updateLibraryAddButtonState();
        selector.selectItems([], selector.selected_background);
    },

    toggleEditMode()
    {
        if(main.main.classList.contains("edit_mode"))
            this.setViewMode();
        else
            this.setEditMode();
    },

    selectItem(foreground, background)
    {
        if(background != null)
        {
            let group = network.dict[background];
            if(main.active_pane && main.active_pane.root)
                main.setPaneBackground(main.active_pane.root, background);
            main.addComponents(group, foreground, background);
            main.refreshMatchingBackgroundPanes(background, foreground, main.active_pane ? main.active_pane.root : null);
        }
        main.updateAutoRoutingButtonState();
        main.savePaneLayout();
    },

    selectCurrentGroupComponents(options = {})
    {
        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group)
            return;

        const includeWidgets = options.includeWidgets !== false;
        const widgetsOnly = options.widgetsOnly === true;
        let components = [];

        if(widgetsOnly)
            components = [...(group.widgets || [])];
        else
        {
            components = [
                ...(group.groups || []),
                ...(group.modules || []),
                ...(group.inputs || []),
                ...(group.outputs || [])
            ];
            if(includeWidgets)
                components.push(...(group.widgets || []));
        }

        selector.selectItems(components.map((component) => background + '.' + component.name), null, false, true);
    },

    // Keyboard shortcuts and command help.
    keydown(evt)
    {
        const key = (evt.key || "").toLowerCase();
        const code = evt.code || "";
        const isModifier = evt.metaKey || evt.ctrlKey;
        const isSaveShortcut = isModifier && (key == "s" || code == "KeyS");

        const activeElement = document.activeElement;
        if(!isSaveShortcut && main.isFormEditingTarget(activeElement))
            return;

        if(main.edit_mode && selector.selected_foreground.length > 0)
        {
            const keyMoves = {
                ArrowLeft: [-1, 0],
                ArrowRight: [1, 0],
                ArrowUp: [0, -1],
                ArrowDown: [0, 1]
            };

            const move = keyMoves[evt.key];
            if(move)
            {
                evt.preventDefault();
                for(let c of selector.selected_foreground)
                    main.changeComponentPosition(c, move[0], move[1], false);
                main.addConnections();
                main.refreshMatchingBackgroundPanes(selector.selected_background, selector.selected_foreground, main.active_pane ? main.active_pane.root : null);
                controller.setTainted(true, "keydown moveComponents");
                return;
            }
        }

        if(evt.key== "Escape")
        {
            main.toggleTopChrome();
            if(typeof inspector !== "undefined" && typeof inspector.hideAllPanels === "function")
                inspector.hideAllPanels();
            evt.preventDefault();
            return;
        }

        if(evt.key== "Backspace" || evt.key == "Delete")
        {
            if(main.edit_mode && (selector.selected_connection != null || selector.selected_foreground.length > 0))
            {
                evt.preventDefault();
                main.deleteComponent();
                return;
            }
            return;
        }

        if(!isModifier)
            return;
        if(key=="a" || code=="KeyA")
        {
            evt.preventDefault();
            if(evt.altKey)
                main.selectCurrentGroupComponents({widgetsOnly: true});
            else if(evt.shiftKey)
                main.selectCurrentGroupComponents({includeWidgets: false});
            else
                main.selectCurrentGroupComponents({includeWidgets: true});
            return;
        }

        else if(key=="c" || code=="KeyC")
        {
            if(main.edit_mode && selector.selected_foreground.length > 0)
            {
                evt.preventDefault();
                main.copySelectedComponents();
            }
            return;
        }
        else if(key=="x" || code=="KeyX")
        {
            if(main.edit_mode && selector.selected_foreground.length > 0)
            {
                evt.preventDefault();
                main.cutSelectedComponents();
            }
            return;
        }
        else if(key=="v" || code=="KeyV")
        {
            if(main.edit_mode && main.component_clipboard && main.component_clipboard.type === "ikaros/components")
            {
                evt.preventDefault();
                main.pasteComponents();
            }
            return;
        }

        else if (key=="d" || code=="KeyD")
        {
            evt.preventDefault();
            if(main.edit_mode)
                main.duplicateSelectedComponents(evt.shiftKey);
            return;
        }
        else if (key=="e" || code=="KeyE")
        {
            evt.preventDefault();
            main.toggleEditMode();
            return;
        }
        else if (key=="i" || code=="KeyI")
        { 
            evt.preventDefault();
            inspector.toggleComponent();
            return;
        }
        else if (key=="s" || code=="KeyS")
        {
            evt.preventDefault();
            if(evt.shiftKey)
                return;
            controller.save();
            return;
        }
        else if (evt.altKey && (key=="t" || code=="KeyT"))
        {
            evt.preventDefault();
            controller.realtime();
            return;
        }
        else if (evt.altKey && (key=="p" || code=="KeyP"))
        {
            evt.preventDefault();
            controller.play();
            return;
        }
        else if (evt.altKey && (key=="." || code=="Period"))
        {
            evt.preventDefault();
            controller.stop();
            return;
        }
        else if (evt.altKey && (key=="n" || code=="KeyN"))
        {
            evt.preventDefault();
            controller.new();
            return;
        }
        else if (evt.altKey && (key=="o" || code=="KeyO"))
        {
            evt.preventDefault();
            controller.open();
            return;
        }
    },

    showCommandInfo()
    {
        dialog.showInfoDialog(
`<table>
    <thead>
        <tr>
            <th>Command</th>
            <th>Effect</th>
        </tr>
    </thead>
    <tbody>
        <tr><td>Arrow keys</td><td>Move selected components by 1 px (edit mode).</td></tr>
        <tr><td>Backspace</td><td>Delete selected components/connections (edit mode).</td></tr>
        <tr><td>Escape</td><td>Toggle top toolbar and breadcrumbs.</td></tr>
        <tr><td>&#8984; + A</td><td>Select all components/widgets in current group.</td></tr>
        <tr><td>&#8984; + &#8679; + A</td><td>Select all non-widget components in current group.</td></tr>
        <tr><td>&#8984; + &#8997; + A</td><td>Select widgets only in current group.</td></tr>
        <tr><td>&#8984; + D</td><td>Duplicate selected components (edit mode).</td></tr>
        <tr><td>&#8984; + &#8679; + D</td><td>Duplicate selected components (edit mode), preserving incoming/outgoing connections.</td></tr>
        <tr><td>&#8984; + E</td><td>Toggle edit mode.</td></tr>
        <tr><td>&#8984; + I</td><td>Toggle inspector.</td></tr>
        <tr><td>&#8984; + S</td><td>Save.</td></tr>
        <tr><td>&#8984; + &#8997; + T</td><td>Set runtime mode (same as Realtime button).</td></tr>
        <tr><td>&#8984; + &#8997; + P</td><td>Set play mode (same as Play button).</td></tr>
        <tr><td>&#8984; + &#8997; + .</td><td>Set stop mode (same as Stop button).</td></tr>
        <tr><td>&#8984; + &#8997; + N</td><td>New.</td></tr>
        <tr><td>&#8984; + &#8997; + O</td><td>Open.</td></tr>
    </tbody>
</table>`,
            "Keyboard Commands",
            true
        );
    }
}
