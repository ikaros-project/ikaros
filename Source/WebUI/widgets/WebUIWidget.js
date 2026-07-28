class WebUIWidget extends HTMLElement
{
    static normalizeTemplate(template)
    {
        const source = Array.isArray(template) ? template : [];
        const frameFieldNames = new Set([
            "show_title",
            "show_frame",
            "background",
            "frame_color",
            "frame_width",
            "style",
            "frame-style"
        ]);
        const frameDefaults = {
            show_title: false,
            show_frame: false,
            background: "",
            frame_color: "",
            frame_width: "1",
            style: "",
            "frame-style": ""
        };
        const frameTypes = {
            show_title: "bool",
            show_frame: "bool",
            background: "string",
            frame_color: "string",
            frame_width: "int",
            style: "string",
            "frame-style": "string"
        };
        const frameControls = {
            show_title: "checkbox",
            show_frame: "checkbox",
            background: "textedit",
            frame_color: "textedit",
            frame_width: "textedit",
            style: "textedit",
            "frame-style": "textedit"
        };

        const existing = {};
        const normalized = [];
        for(const entry of source)
        {
            if(!entry || typeof entry !== "object")
                continue;
            if(entry.control === "header" && entry.name === "FRAME")
                continue;
            if(frameFieldNames.has(entry.name))
            {
                existing[entry.name] = entry;
                continue;
            }
            normalized.push(entry);
        }

        normalized.push({name: "FRAME", control: "header"});
        for(const name of ["show_title", "show_frame", "background", "frame_color", "frame_width", "style", "frame-style"])
        {
            const prior = existing[name] || {};
            normalized.push({
                ...prior,
                name,
                default: prior.default !== undefined ? prior.default : frameDefaults[name],
                type: prior.type || frameTypes[name],
                control: prior.control || frameControls[name]
            });
        }

        return normalized;
    }

    // functions that should be overridden in subclasses
    
    requestData(data_set)
    {
    }

    static html()
    {
        return `
            <style>
                div { background-color: rgba(0,0,0,0); color: red; }
            </style>
            <div>Widget</div>
        `;
    }

    static template()
    {
        return []
    };

    // top level function

    setType(x, t)
    {
        if(t == 'int')
            return parseInt(x);
        
        if(t == 'float')
            return parseFloat(x);
        
        if(t == 'bool')
            return x !== undefined && x !== null && ['on','yes','true','1'].includes(x.toString().toLowerCase());
        
        return x;
    }

    constructor()
    {
        super();
        let pt = this.constructor.template();
        pt = this.constructor.normalizeTemplate(pt);
        this.param_types = {};
        this.parameters = {};
        for(let i in pt)
            if(pt[i].control != 'header')
            {
                this.parameters[pt[i].name] = this.setType(pt[i]['default'], pt[i]['type']);
                this.param_types[pt[i].name] = pt[i]['type'];
            }
        this.parameter_template = pt;        
     }

    get(url, callback) // FIXME: This function should instead call the get function in webui.js to maintain update
    {
        controller.get(url, controller.update);
        return;
        
        var last_request = url;

        xhr = new XMLHttpRequest();
        xhr.open("GET", url, true);

        xhr.onloadstart = function(evt)
        {
            document.querySelector("progress").setAttribute("value", 0);
        }

        xhr.onprogress = function(evt)
        {
            if (evt.lengthComputable)
            {
                var percentComplete = evt.loaded / evt.total;
//                console.log("Progress: "+parseInt(100*percentComplete)+"% complete");
                document.querySelector("progress").setAttribute("value", 100*percentComplete);
            }
        }
        xhr.onerror = function(evt)
        {
            console.log("onerror");
//            console.log(evt);
            if(evt.lengthComputable && evt.loaded < evt.total)
                console.log("Failed to load resource. Incomplete.");
            else if(evt.total == 0 )
                console.log("Failed to load resource. No data.");
            else
                console.log("Failed to load resource.");
 
//            console.log("Resending request");
//            controller.get(last_request, controller.update);
       }
        xhr.ontimeout = function(evt)
        {
            console.log("Timeout - resending request");
//            console.log(evt);
            
            // Resend request
            
//            controller.get(last_request, controller.update);
        }
        xhr.onload = function(evt)
        {
    //        console.log("The transfer is complete.");
    //        console.log(xhr.response);
            if(callback)
                callback(xhr.response, xhr.getResponseHeader("Session-Id"));
        }
        
        xhr.responseType = 'json';
        xhr.timeout = 1000;
        xhr.send();
    }


    getSource(source, default_data=undefined)
    {
        try {
            let v = this.receivedData[this.resolveControlPath(this.parameters[source])];

            // FIXME: This code should be in individual Widgets if necessary
            //if(v != undefined && typeof v[0] != "object") // FIXME: Temporary fix for arrays
             //   v = [v];

            if(v !== undefined && v !== null)
                return v;
            else
                return default_data;
        }
        catch(err)
        {
            return default_data;
        }
    }

    getSourceMetadata(source, default_data=undefined)
    {
        try {
            const resolvedSource = this.resolveControlPath(this.parameters[source]);
            if(!resolvedSource)
                return default_data;

            let v = this.receivedData[`${resolvedSource}:metadata`];
            return v !== undefined && v !== null ? v : default_data;
        }
        catch(err)
        {
            return default_data;
        }
    }
    
    getSourceAsArray(source, default_array=[])
    {
        return this.getSource(source, [default_array])[0];
    }

    getSourceAsFloat(source, default_value=0)
    {
        return parseFloat(this.getSource(source, [[default_value]])[0][0]);
    }

    addSource(data_set, source) // this will be default function for all widgets later
    {
        if(source)
            data_set.add(this.resolveControlPath(source));
    }

    addSourceMetadata(data_set, source)
    {
        if(source)
            data_set.add(`${this.resolveControlPath(source)}:metadata`);
    }


    requestData(data_set) // this is the default function for all widgets
    {
        this.parameter_template.filter(p => p.type == "source").forEach(p => { this.addSource(data_set, this.parameters[p.name]) });
    }

    // ACCESS FUNCTIONS: Needs some cleanup

    // getProp finds a format variable by first looking through the attributes of the widget
    // and then at the variables set in CSS

    getProp(attribute, index)
    {
        try
        {
            let v = getComputedStyle(this).getPropertyValue(attribute);
            if(index !== undefined)
                return v.split(",")[index].trim();
            else
                return v.trim();
        }
        catch(err)
        {
            return undefined;
        }
    }

    getMatrixRank(matrix) 
    {
        let rank = 0;

        while (Array.isArray(matrix)) {
            rank++;
            matrix = matrix[0];
        }
        return rank;
    }

    
    getInt(attribute, index)
    {
        try
        {
            if(index !== undefined)
                return parseInt(this.getProp(attribute).split(",")[index]);
            else
                return parseInt(this.getProp(attribute));
        }
        catch(err)
        {
            return 0;
        }
    }

    getFloat(attribute, index)
    {
        try
        {
            if(index !== undefined)
                return parseFloat(this.getProp(attribute).split(",")[index]);
            else
                return parseFloat(this.getProp(attribute));
        }
        catch(err)
        {
            return 0;
        }
    }

    toBool(x)
    {
        return x !== undefined && x !== null && ['on','yes','true','1'].includes(x.toString().toLowerCase());
    }

    getBool(attribute, index)
    {
        try
        {
            if(index !== undefined)
                return ['yes','true','on','1'].includes(this.getProp(attribute).split(",")[index].toLowerCase());
            else
                return ['yes','true','on','1'].includes(this.getProp(attribute).toLowerCase());
        }
        catch(err)
        {
            return false;
        }
    }

    getOfType(attribute, index, type)
    {
        if(type == 'bool')
            return this.getBool(attribute, index)
        else if(type == 'int')
            return this.getInt(attribute, index)
        else if(type == 'float')
            return this.getFloat(attribute, index)
        else
            return this.getProp(attribute, index)
    }

    setFormat(variable, attribute, type, index=undefined)
    {
        let v = null;
        if(variable in this.parameters && this.parameters[variable] != "")  // use style if parameter has no value (or no default)
        {
            if(index !== undefined)
                v = String(this.parameters[variable]).split(",")[index].toLowerCase()
            else
                v = this.parameters[variable];
            v = this.setType(v, type);
        }
        else
        {
            v = this.getOfType(attribute, index, type)
        }
        
        this.format[variable] = v;
    }

    readCSSvariables()
    {
        this.format = {}

        this.setFormat('direction', '--direction', 'string');

        this.setFormat('titleHeight', '--title-height', 'int');
        this.setFormat('titleFont', '--title-font', 'string');
        this.setFormat('titleColor', '--title-color', 'string');
        this.setFormat('titleBackground', '--title-background', 'string');
        this.setFormat('titleMargins', '--title-margins','int');
        this.setFormat('titleAlign', '--title-align', 'string');
    //    this.setFormat('ViewX', '--title-offset','int', 0);
    //    this.setFormat('ViewY', '--title-offset','int', 1);

        this.setFormat('margin_left', '--margin-left', 'int');
        this.setFormat('margin_right', '--margin-right', 'int');
        this.setFormat('margin_top', '--margin-top', 'int');
        this.setFormat('margin_bottom', '--margin-bottom', 'int');

        this.setFormat('space_left', '--space-left', 'int');
        this.setFormat('space_right', '--space-right', 'int');
        this.setFormat('space_top', '--space-top', 'int');
        this.setFormat('space_bottom', '--space-bottom', 'int');

        this.setFormat('spacing', '--spacing', 'int');

        this.setFormat('stroke_color', '--color', 'string')
        this.setFormat('positiveColor', '--positive-color', 'string');
        this.setFormat('negativeColor', '--negative-color', 'string');
        this.setFormat('line_width', '--line-width', 'string');
        this.setFormat('line_dash', '--line-dash', 'string');
        this.setFormat('line_cap', '--line-cap', 'string');
        this.setFormat('line_join', '--line-join', 'string');
        this.setFormat('close', '--close', 'bool');
        this.setFormat('arrow', '--arrow', 'bool');
        this.setFormat('fill_color', '--fill', 'string');

        this.setFormat('grid_color', '--grid-color', 'string');
        this.setFormat('grid_line_width', '--grid-line-width', 'string');
        this.setFormat('grid_fill', '--grid-fill', 'string');

        this.setFormat('flip_x_axis', '--flip-x-axis', 'bool');
        this.setFormat('flip_y_axis', '--flip-y-axis', 'bool');
        this.setFormat('flip_x_canvas', '--flip-x-canvas', 'bool');
        this.setFormat('flip_y_canvas', '--flip-y-canvas', 'bool');

        this.setFormat('frame', '--frame', 'string');
        this.setFormat('show_x_axis', '--x-axis', 'bool');
        this.setFormat('show_y_axis', '--y-axis', 'bool');
        this.setFormat('axis_color', '--axis-color', 'string');
        this.setFormat('vertical_grid_lines', '--vertical-gridlines', 'int');
        this.setFormat('horizontal_grid_lines', '--horizontal-gridlines', 'int');
        this.setFormat('vertical_grid_lines_over', '--vertical-gridlines-over', 'int');
        this.setFormat('horizontal_grid_lines_over', '--horizontal-gridlines-over', 'int');
        this.setFormat('left_tick_marks', '--left-tick-marks', 'int');
        this.setFormat('right_tick_marks', '--right-tick-marks', 'int');
        this.setFormat('bottom_tick_marks', '--bottom-tick-marks', 'int');
        this.setFormat('left_scale_ticks', '--left-scale', 'int');
        this.setFormat('right_scale_ticks', '--right-scale', 'int');
        this.setFormat('bottom_scale_ticks', '--bottom-scale', 'int');
        this.setFormat('scale_offset', '--scale-offset', 'int');
        this.setFormat('scale_font', '--scale-font', 'string');

        this.setFormat('labels', '--labels', 'bool');
        this.setFormat('labelColor', '--label-color', 'string');
        this.setFormat('label_font', '--label-font', 'string');
        this.setFormat('show_x_labels', '--draw-labels-x', 'bool');
        this.setFormat('show_y_labels', '--draw-labels-y', 'bool');
        
        this.setFormat('decimals', '--decimals', 'int');

        this.setFormat('min', '--min', 'float');
        this.setFormat('max', '--max', 'float');
    }

    getColor(i, v)
    {
        try
        {
            if(v !== undefined && v !== null && v >= 0 && this.format.positiveColor)
            {
                let l = this.format.positiveColor.split(",");
                let n = l.length;
                return l[i % n].trim();
            }
            else if(v && this.format.negativeColor)
            {
                let l = this.format.negativeColor.split(",");
                let n = l.length;
                return l[i % n].trim();
            }
            else
            {
                let l = this.format.stroke_color.split(",");
                let n = l.length;
                return l[i % n].trim();
            }
        }
        catch(err)
        {
            return "black";
        }
    }

    setCSSClass()
    {
        this.className = "widget "
        for(let p of this.parameter_template)
            if(p['class'])
                this.className += p['name']+'-'+this.parameters[p['name']] + " "
    }
    
    connectedCallback()
    {
        this.innerHTML = this.constructor.html();

        // These are for documentation purposes:
/*
        this.onmousedown = function () { console.log("WebUIWidgetCanvas: mouse down"); }
        this.onmouseup = function () { console.log("WebUIWidgetCanvas: mouse up"); }
        this.onclick = function () { console.log("WebUIWidgetCanvas: click"); }
        this.onmousemove = function () { console.log("WebUIWidgetCanvas: mousemove"); }
        this.onmouseover = function () { console.log("WebUIWidgetCanvas: mouseover"); }
        this.onmouseout = function () { console.log("WebUIWidgetCanvas: mouseout"); }
*/

        // Default onclick function - send click coordinate if command is set
/*
        this.onclick = function (evt)
        {
            return;
           if(this.parameters.command)
           {
                let lw = this.parameters.labels ? parseInt(this.parameters.label_width) : 0;
                let r = this.canvasElement.getBoundingClientRect();
                let x = (evt.clientX - r.left - this.format.space_left - lw)/(r.width - this.format.space_left - this.format.space_right- lw);
                let y = (evt.clientY - r.top - this.format.space_top)/(r.height - this.format.space_top - this.format.space_bottom);
                send_command(this.parameters.command, 1, x, y);
            }
            else
            {
                alert("!");
            }
         }
 */

        this.updateStyle(this, this.parameters['style']);
        this.updateStyle(this.parentNode, this.parameters['frame-style']);
        this.readCSSvariables();

        // set classes for formating

        this.init();
    }

    updateStyle(element, style)
    {
        if(!style)
            return;
        for(let r of style.split(';'))
            try
            {
                let p;
                let v;
                [p, v] = r.split(':');
                element.style.setProperty(p.trim(), v.trim());
            }
        catch(err)
        {
            return false;
        }
        this.readCSSvariables();
    }

    updateFrame()
    {
        let fcolors = String(this.parameters.frame_color ?? "").split(',').map((c) => c.trim()).filter((c) => c !== "");
        if(fcolors.length > 0)
        {
            this.parentElement.style.borderTopColor = fcolors[0];
            this.parentElement.style.borderRightColor = fcolors[1 % fcolors.length];
            this.parentElement.style.borderBottomColor = fcolors[2 % fcolors.length];
            this.parentElement.style.borderLeftColor = fcolors[3 % fcolors.length];
        }
        else
        {
            this.parentElement.style.borderTopColor = "";
            this.parentElement.style.borderRightColor = "";
            this.parentElement.style.borderBottomColor = "";
            this.parentElement.style.borderLeftColor = "";
        }

        let fw = this.parameters.frame_width;
        this.parentElement.style.borderWidth = fw ? fw + "px" : "";
        this.parentElement.style.background = this.parameters.background;
        this.parentElement.classList.toggle('visible', this.toBool(this.parameters.show_frame));
        const titleContainer = this.parentElement.firstChild;
        titleContainer.style.display = this.toBool(this.parameters.show_title) ? 'block' : 'none';

        let titleText = titleContainer.querySelector(".component-title-text");
        if(!titleText)
        {
            titleText = document.createElement("span");
            titleText.className = "component-title-text";
            const fullName = this.parentElement.dataset ? this.parentElement.dataset.name : "";
            if(fullName)
                titleText.dataset.component = fullName;
            titleText.dataset.field = "title";
            titleContainer.replaceChildren(titleText);
        }

        if(!titleText.classList.contains("inline-name-edit"))
            titleText.textContent = this.parameters.title;

        this.setCSSClass();
        this.readCSSvariables();
    }

    init()
    {
        this.updateAll();
    }

    update()
    {
    }

    updateAll()
    {
        this.updateFrame();
        this.update();
    }

    parameterChangeNotification(p)
    {
        this.updateAll();
    }

    getWidgetGroupPath()
    {
        const fullName = this.parentElement && this.parentElement.dataset ? this.parentElement.dataset.name : "";
        if(fullName.includes("."))
            return fullName.substring(0, fullName.lastIndexOf("."));

        if(typeof selector !== "undefined" && selector && selector.selected_background)
            return selector.selected_background;

        return "";
    }

    getControlScopePath()
    {
        const groupPath = this.getWidgetGroupPath();
        if(
            typeof network !== "undefined" &&
            network &&
            network.dict &&
            groupPath &&
            network.dict[groupPath] &&
            typeof network.dict[groupPath].proxy === "string"
        )
        {
            const proxyPath = network.dict[groupPath].proxy.trim();
            if(proxyPath !== "")
                return proxyPath[0] === "." ? proxyPath.substring(1) : proxyPath;
        }

        return groupPath;
    }

    resolveControlPath(path)
    {
        if(typeof path !== "string")
            return path;

        const trimmedPath = path.trim();
        if(trimmedPath === "" || trimmedPath[0] === ".")
            return trimmedPath;

        const scopePath = this.getControlScopePath();
        if(scopePath === "")
            return trimmedPath;

        return `.${scopePath}.${trimmedPath}`;
    }

    send_control_change(parameter, value=0, index_x=0, index_y=0)
    {
        if(main.edit_mode)
            return;
        controller.queueCommand("control", this.resolveControlPath(parameter), {"x":index_x, "y":index_y, "value":value});
        // controller.queueCommand("control", parameter.substring(0, parameter.lastIndexOf('.')), {"x":index_x, "y":index_y, "value":value});     

    }

    send_command(command, value=0, index_x=0, index_y=0)
    {
        const resolvedCommand = this.resolveControlPath(command);
        let path =  resolvedCommand.substring(0, resolvedCommand.lastIndexOf('.'));
        let name = resolvedCommand.substring(resolvedCommand.lastIndexOf('.') + 1);
        controller.queueCommand("command", path, {"command":name, "x":index_x, "y":index_y, "value":value}); 
    }

    widget_loading(state)
    {
        if (state && !this.loading)
        {
            console.log("Add Loading")
            this.insertAdjacentHTML("afterbegin", '<section id="loading-screen"><div id="loader"></div></section>')
            this.loading = true;

        }
        if (!state && this.loading)
        {
            console.log("Remove Loading")
            this.querySelector('#loading-screen').classList.add( 'fade-out' );
			this.querySelector('#loading-screen').addEventListener( 'transitionend', onTransitionEnd );
            this.loading = false;

        }
        function onTransitionEnd( event ) {
			event.target.remove();
		}
    }

    widget_overlay(state, text = "")
    {
        if (state && !this.overlay)
        {
            console.log("Add overlay")
            this.insertAdjacentHTML("afterbegin", '<section id="overlay-screen"><div id="overlay">' + text + '</div></section>')
            this.overlay = true;
        }
        if (!state && this.overlay)
        {
            console.log("Remove overlay")
            this.querySelector('#overlay-screen').classList.add( 'fade-out' );
			this.querySelector('#overlay-screen').addEventListener( 'transitionend', onTransitionEnd );
            this.overlay = false;
        }
        function onTransitionEnd( event ) {
			event.target.remove();
		}
    }
};

customElements.define('webui-widget', WebUIWidget);
