Object.assign(main, {
    getPaneComponentElement(pane, logicalId)
    {
        if(!pane || !logicalId)
            return null;
        return main.getPaneElement(logicalId, pane);
    },

    getElementsByLogicalId(logicalId, root=document)
    {
        if(!logicalId || !root || !root.querySelectorAll)
            return [];
        return Array.from(root.querySelectorAll("[data-logical-id]")).filter((element) => element.dataset.logicalId === logicalId);
    },

    getEndpointViewportPosition(endpointId, pane)
    {
        const element = main.getPaneComponentElement(pane, endpointId);
        if(!element)
            return null;

        const rect = element.getBoundingClientRect();
        return {
            pane,
            element,
            endpointId,
            x: rect.left + rect.width / 2,
            y: rect.top + rect.height / 2,
            rect
        };
    },

    getVisibleEndpointPlacements(endpointId, panes=null)
    {
        const candidatePanes = panes || main.getAllPanes();
        const placements = [];
        for(const pane of candidatePanes)
        {
            const placement = main.getEndpointViewportPosition(endpointId, pane);
            if(placement)
                placements.push(placement);
            else
            {
                const boundaryPlacement = main.getBoundaryEndpointViewportPosition(endpointId, pane);
                if(boundaryPlacement)
                    placements.push(boundaryPlacement);
            }
        }
        return placements;
    },

    getBoundaryEndpointViewportPosition(endpointId, pane)
    {
        const paneBackground = main.getPaneBackground(pane);
        if(!endpointId || !paneBackground || !endpointId.startsWith(`${paneBackground}.`))
            return null;

        const suffix = endpointId.endsWith(":out") ? ":out" : (endpointId.endsWith(":in") ? ":in" : "");
        if(!suffix)
            return null;

        const baseEndpoint = endpointId.slice(0, -suffix.length);
        const relativeEndpoint = baseEndpoint.slice(paneBackground.length + 1);
        if(!relativeEndpoint || relativeEndpoint.includes("."))
            return null;

        const boundarySuffix = suffix === ":out" ? ":in" : ":out";
        const boundaryEndpointId = `${baseEndpoint}${boundarySuffix}`;
        const placement = main.getEndpointViewportPosition(boundaryEndpointId, pane);
        if(!placement)
            return null;

        placement.endpointId = endpointId;
        placement.boundaryEndpointId = boundaryEndpointId;
        placement.isBoundaryEndpoint = true;
        return placement;
    },

    getConnectionEndpointIds(path, connection)
    {
        const sockets = main.getConnectionSocketIds(path, connection);
        return {
            sourceEndpointId: sockets.sourceSocketId,
            targetEndpointId: sockets.targetSocketId
        };
    },

    getConnectionEndpointBase(endpointId)
    {
        return getStringUpToBracket((endpointId || "").split(":")[0] || "");
    },

    getConnectionOwnerPath(sourceEndpointId, targetEndpointId)
    {
        const sourceParts = main.getConnectionEndpointBase(sourceEndpointId).split(".").filter(Boolean);
        const targetParts = main.getConnectionEndpointBase(targetEndpointId).split(".").filter(Boolean);
        const commonParts = [];
        for(let i = 0; i < Math.min(sourceParts.length, targetParts.length); i++)
        {
            if(sourceParts[i] !== targetParts[i])
                break;
            commonParts.push(sourceParts[i]);
        }

        while(commonParts.length > 0)
        {
            const path = commonParts.join(".");
            const item = network && network.dict ? network.dict[path] : null;
            if(item && item._tag === "group")
                return path;
            commonParts.pop();
        }

        return selector ? selector.selected_background : "";
    },

    getRelativeConnectionEndpoint(endpointId, ownerPath)
    {
        const endpointBase = main.getConnectionEndpointBase(endpointId);
        return removeStringFromStart(endpointBase, `${ownerPath}.`);
    },

    resolveConnectionId(connectionId)
    {
        if(!connectionId || !network || !network.dict)
            return null;

        const parts = connectionId.split("*");
        if(parts.length !== 2)
            return null;

        for(const path in network.dict)
        {
            const group = network.dict[path];
            if(!group || group._tag !== "group" || !Array.isArray(group.connections))
                continue;

            for(const connection of group.connections)
            {
                if(main.getConnectionKey(path, connection) === connectionId)
                {
                    const endpoints = main.getConnectionEndpointIds(path, connection);
                    return {
                        id: connectionId,
                        path,
                        group,
                        connection,
                        source: connection.source,
                        target: connection.target,
                        sourceEndpointId: endpoints.sourceEndpointId,
                        targetEndpointId: endpoints.targetEndpointId
                    };
                }
            }
        }

        const fallbackPath = main.getConnectionOwnerPath(parts[0], parts[1]);
        return {
            id: connectionId,
            path: fallbackPath,
            group: null,
            connection: null,
            source: main.getRelativeConnectionEndpoint(parts[0], fallbackPath),
            target: main.getRelativeConnectionEndpoint(parts[1], fallbackPath),
            sourceEndpointId: `${parts[0]}:out`,
            targetEndpointId: `${parts[1]}:in`
        };
    },

    getPanesForConnectionPath(path)
    {
        if(!path)
            return [];
        return main.getAllPanes().filter((pane) =>
        {
            const background = main.getPaneBackground(pane);
            return background === path || background.startsWith(`${path}.`);
        });
    },

    getVisibleConnectionEndpoints(path, connection, panes=null)
    {
        const endpointIds = main.getConnectionEndpointIds(path, connection);
        const candidatePanes = panes || main.getPanesForConnectionPath(path);
        return {
            source: main.getVisibleEndpointPlacements(endpointIds.sourceEndpointId, candidatePanes),
            target: main.getVisibleEndpointPlacements(endpointIds.targetEndpointId, candidatePanes),
            ...endpointIds
        };
    },

    classifyConnectionVisibility(path, connection, panes=null)
    {
        const endpoints = main.getVisibleConnectionEndpoints(path, connection, panes);
        const samePanes = endpoints.source
            .map((sourcePlacement) => sourcePlacement.pane)
            .filter((pane) => endpoints.target.some((targetPlacement) => targetPlacement.pane === pane));

        let classification = "hidden";
        if(samePanes.length > 0)
            classification = "same-pane";
        else if(endpoints.source.length > 0 && endpoints.target.length > 0)
            classification = "cross-pane";

        return {
            classification,
            samePanes,
            ...endpoints
        };
    },

    findCrossPaneEndpointPair(visibility)
    {
        if(!visibility || visibility.classification !== "cross-pane")
            return null;
        for(const source of visibility.source || [])
        {
            for(const target of visibility.target || [])
            {
                if(source.pane !== target.pane)
                    return {source, target};
            }
        }
        return null;
    },

    findCrossPaneEndpointPairs(visibility)
    {
        if(!visibility)
            return [];

        const pairs = [];
        for(const source of visibility.source || [])
        {
            for(const target of visibility.target || [])
            {
                if(source.pane !== target.pane)
                    pairs.push({source, target});
            }
        }
        return pairs;
    },

    createWorkspaceConnectionLayer()
    {
        if(main.workspace_connection_layer && main.workspace_connection_layer.parentElement)
            return main.workspace_connection_layer;
        if(!main.main)
            return null;

        const layer = document.createElement("div");
        layer.className = "main_workspace_connection_layer";
        layer.innerHTML = "<svg xmlns='http://www.w3.org/2000/svg' class='workspace_connections_svg'></svg>";
        main.main.appendChild(layer);
        main.workspace_connection_layer = layer;
        main.workspace_connection_svg = layer.querySelector(".workspace_connections_svg");
        return layer;
    },

    observeWorkspaceLayout()
    {
        if(main.workspace_resize_observer || typeof ResizeObserver === "undefined")
            return;

        main.workspace_resize_observer = new ResizeObserver(() => main.scheduleWorkspaceConnectionUpdate());
        if(main.main)
            main.workspace_resize_observer.observe(main.main);
        if(main.workspace)
            main.workspace_resize_observer.observe(main.workspace);
    },

    scheduleWorkspaceConnectionUpdate()
    {
        if(main.workspace_connection_update_frame)
            cancelAnimationFrame(main.workspace_connection_update_frame);
        main.workspace_connection_update_frame = requestAnimationFrame(() =>
        {
            main.workspace_connection_update_frame = null;
            main.updateWorkspaceConnections();
        });
    },

    getWorkspaceConnectionLayer()
    {
        return main.workspace_connection_layer && main.workspace_connection_layer.parentElement
            ? main.workspace_connection_layer
            : main.createWorkspaceConnectionLayer();
    },

    getWorkspaceConnectionSvg()
    {
        const layer = main.getWorkspaceConnectionLayer();
        if(!layer)
            return null;
        if(!main.workspace_connection_svg || !layer.contains(main.workspace_connection_svg))
            main.workspace_connection_svg = layer.querySelector(".workspace_connections_svg");
        return main.workspace_connection_svg;
    },

    clearWorkspaceConnections()
    {
        const svg = main.getWorkspaceConnectionSvg();
        if(svg)
            svg.innerHTML = "";
    },

    drawWorkspaceConnectionLine(path, connection, sourcePlacement, targetPlacement, options={})
    {
        const svg = main.getWorkspaceConnectionSvg();
        if(!svg || !sourcePlacement || !targetPlacement || !main.main)
            return;

        const mainRect = main.main.getBoundingClientRect();
        const connectionId = main.getConnectionKey(path, connection);
        const x1 = String(sourcePlacement.x - mainRect.left);
        const y1 = String(sourcePlacement.y - mainRect.top);
        const x2 = String(targetPlacement.x - mainRect.left);
        const y2 = String(targetPlacement.y - mainRect.top);

        const hitLine = document.createElementNS("http://www.w3.org/2000/svg", "line");
        hitLine.setAttribute("x1", x1);
        hitLine.setAttribute("y1", y1);
        hitLine.setAttribute("x2", x2);
        hitLine.setAttribute("y2", y2);
        hitLine.setAttribute("class", "workspace_connection_hit_line");
        hitLine.setAttribute("data-logical-id", connectionId);
        hitLine.addEventListener("click", () => selector.selectConnectionAndReveal(connectionId), false);
        hitLine.addEventListener("dblclick", () => selector.selectConnectionAndReveal(connectionId), false);

        const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
        const connectionColor = main.getConnectionColorValue(connection);
        const occurrenceIndex = Number.isFinite(options.occurrenceIndex) ? options.occurrenceIndex : 0;
        const occurrenceCount = Number.isFinite(options.occurrenceCount) ? options.occurrenceCount : 1;
        line.setAttribute("x1", x1);
        line.setAttribute("y1", y1);
        line.setAttribute("x2", x2);
        line.setAttribute("y2", y2);
        line.setAttribute("class", "connection_line workspace_connection_line");
        line.setAttribute("data-logical-id", connectionId);
        line.setAttribute("data-source", connection.source || "");
        line.setAttribute("data-target", connection.target || "");
        line.setAttribute("data-occurrence-index", String(occurrenceIndex));
        line.setAttribute("data-occurrence-count", String(occurrenceCount));
        if(selector && selector.selected_connection === connectionId)
            line.classList.add("selected");
        line.addEventListener("click", () => selector.selectConnectionAndReveal(connectionId), false);
        line.addEventListener("dblclick", () => selector.selectConnectionAndReveal(connectionId), false);
        if(connectionColor)
            line.style.setProperty("--connection-color", connectionColor);
        const setHover = (hover) => line.classList.toggle("hover", hover);
        hitLine.addEventListener("mouseenter", () => setHover(true), false);
        hitLine.addEventListener("mouseleave", () => setHover(false), false);
        svg.appendChild(hitLine);
        svg.appendChild(line);
    },

    drawTrackedWorkspaceConnection()
    {
        const tracked = main.tracked_connection;
        const svg = main.getWorkspaceConnectionSvg();
        if(!tracked || !svg || !main.main)
            return;

        const mainRect = main.main.getBoundingClientRect();
        const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
        line.setAttribute("x1", String(tracked.client_x1 - mainRect.left));
        line.setAttribute("y1", String(tracked.client_y1 - mainRect.top));
        line.setAttribute("x2", String(tracked.client_x2 - mainRect.left));
        line.setAttribute("y2", String(tracked.client_y2 - mainRect.top));
        line.setAttribute("class", "connection_line workspace_connection_line tracked");
        svg.appendChild(line);
    },

    getDistanceToSegment(px, py, x1, y1, x2, y2)
    {
        const dx = x2 - x1;
        const dy = y2 - y1;
        if(dx === 0 && dy === 0)
            return Math.hypot(px - x1, py - y1);

        const t = Math.max(0, Math.min(1, ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy)));
        const x = x1 + t * dx;
        const y = y1 + t * dy;
        return Math.hypot(px - x, py - y);
    },

    getWorkspaceConnectionAtPoint(clientX, clientY)
    {
        if(!main.main)
            return null;

        const mainRect = main.main.getBoundingClientRect();
        let closest = null;
        let closestDistance = Infinity;
        for(const line of document.querySelectorAll(".workspace_connection_line:not(.tracked)"))
        {
            const x1 = mainRect.left + Number(line.getAttribute("x1"));
            const y1 = mainRect.top + Number(line.getAttribute("y1"));
            const x2 = mainRect.left + Number(line.getAttribute("x2"));
            const y2 = mainRect.top + Number(line.getAttribute("y2"));
            const distance = main.getDistanceToSegment(clientX, clientY, x1, y1, x2, y2);
            if(distance < closestDistance)
            {
                closestDistance = distance;
                closest = line;
            }
        }
        return closestDistance <= 6 ? closest : null;
    },

    handleWorkspaceConnectionClick(evt)
    {
        if(!evt || evt.defaultPrevented || main.tracked_connection)
            return;

        const line = main.getWorkspaceConnectionAtPoint(evt.clientX, evt.clientY);
        if(!line || !line.dataset.logicalId)
            return;

        evt.preventDefault();
        evt.stopPropagation();
        selector.selectConnectionAndReveal(line.dataset.logicalId);
    },

    updateWorkspaceConnections()
    {
        main.clearWorkspaceConnections();
        if(!network || !network.dict)
            return;

        const visibleConnections = [];
        const occurrenceCounts = new Map();
        for(const path in network.dict)
        {
            const group = network.dict[path];
            if(!group || group._tag !== "group" || !Array.isArray(group.connections))
                continue;

            for(const connection of group.connections)
            {
                const visibility = main.classifyConnectionVisibility(path, connection);
                // If both endpoints are already visible in any pane, keep the connection local to that pane.
                if(visibility.samePanes.length > 0)
                    continue;
                for(const pair of main.findCrossPaneEndpointPairs(visibility))
                {
                    const id = main.getConnectionKey(path, connection);
                    visibleConnections.push({id, path, connection, source: pair.source, target: pair.target});
                    occurrenceCounts.set(id, (occurrenceCounts.get(id) || 0) + 1);
                }
            }
        }

        const occurrenceIndexes = new Map();
        const sortedConnections = visibleConnections.sort((a, b) =>
        {
            const aSelected = selector && selector.selected_connection === a.id;
            const bSelected = selector && selector.selected_connection === b.id;
            if(aSelected === bSelected)
                return 0;
            return aSelected ? 1 : -1;
        });

        for(const entry of sortedConnections)
        {
            const occurrenceIndex = occurrenceIndexes.get(entry.id) || 0;
            occurrenceIndexes.set(entry.id, occurrenceIndex + 1);
            main.drawWorkspaceConnectionLine(entry.path, entry.connection, entry.source, entry.target, {
                occurrenceIndex,
                occurrenceCount: occurrenceCounts.get(entry.id) || 1
            });
        }
        main.drawTrackedWorkspaceConnection();
    }
});
