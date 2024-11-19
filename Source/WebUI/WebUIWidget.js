class WebUIWidget extends HTMLElement
{
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
            return ['on','yes','true'].includes(x.toString().toLowerCase());
        
        return x;
    }

    constructor()
    {
        super();
        let pt = this.constructor.template();
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

    get(url, callback) // FIXME: This function should instead call the get funciton in webui.js to maintain update
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
            let v = this.receivedData[this.parameters[source]];

            //FIXME: This code should be in individual Widgets if necessary
            if(v != undefined && typeof v[0] != "object") // FIXME: Temporary fix for arrays
               v = [v];

            if(v)
                return v;
            else
                return default_data;
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
            data_set.add(source);
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
            if(index)
                return v.split(",")[index].trim();
            else
                return v.trim();
        }
        catch(err)
        {
            return undefined;
        }
    }

    getInt(attribute, index)
    {
        try
        {
            if(index)
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
            if(index)
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
        return ['on','yes','true','1'].includes(x.toString().toLowerCase());
    }

    getBool(attribute, index)
    {
        try
        {
            if(index)
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
            if(index)
                v = this.parameters[variable].split(",")[index].toLowerCase()
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

        this.setFormat('marginLeft', '--margin-left', 'int');
        this.setFormat('marginRight', '--margin-right', 'int');
        this.setFormat('marginTop', '--margin-top', 'int');
        this.setFormat('marginBottom', '--margin-bottom', 'int');

        this.setFormat('spaceLeft', '--space-left', 'int');
        this.setFormat('spaceRight', '--space-right', 'int');
        this.setFormat('spaceTop', '--space-top', 'int');
        this.setFormat('spaceBottom', '--space-bottom', 'int');

        this.setFormat('spacing', '--spacing', 'int');

        this.setFormat('color', '--color', 'string')
        this.setFormat('positiveColor', '--positive-color', 'string');
        this.setFormat('negativeColor', '--negative-color', 'string');
        this.setFormat('lineWidth', '--line-width', 'string');
        this.setFormat('lineDash', '--line-dash', 'string');
        this.setFormat('lineCap', '--line-cap', 'string');
        this.setFormat('lineJoin', '--line-join', 'string');
        this.setFormat('close', '--close', 'bool');
        this.setFormat('arrow', '--arrow', 'bool');
        this.setFormat('fill', '--fill', 'string');

        this.setFormat('gridColor', '--grid-color', 'string');
        this.setFormat('gridLineWidth', '--grid-line-width', 'string');
        this.setFormat('gridFill', '--grid-fill', 'string');

        this.setFormat('flipXAxis', '--flip-x-axis', 'bool');
        this.setFormat('flipYAxis', '--flip-y-axis', 'bool');
        this.setFormat('flipXCanvas', '--flip-x-canvas', 'bool');
        this.setFormat('flipYCanvas', '--flip-y-canvas', 'bool');

        this.setFormat('frame', '--frame', 'string');
        this.setFormat('xAxis', '--x-axis', 'bool');
        this.setFormat('yAxis', '--y-axis', 'bool');
        this.setFormat('axisColor', '--axis-color', 'string');
        this.setFormat('verticalGridlines', '--vertical-gridlines', 'int');
        this.setFormat('horizontalGridlines', '--horizontal-gridlines', 'int');
        this.setFormat('verticalGridlinesOver', '--vertical-gridlines-over', 'int');
        this.setFormat('horizontalGridlinesOver', '--horizontal-gridlines-over', 'int');
        this.setFormat('leftTickMarks', '--left-tick-marks', 'int');
        this.setFormat('rightTickMarks', '--right-tick-marks', 'int');
        this.setFormat('bottomTickMarks', '--bottom-tick-marks', 'int');
        this.setFormat('leftScale', '--left-scale', 'int');
        this.setFormat('rightScale', '--right-scale', 'int');
        this.setFormat('bottomScale', '--bottom-scale', 'int');
        this.setFormat('scaleOffset', '--scale-offset', 'int');
        this.setFormat('scaleFont', '--scale-font', 'string');

        this.setFormat('labels', '--labels', 'bool');
        this.setFormat('labelColor', '--label-color', 'string');
        this.setFormat('labelFont', '--label-font', 'string');
        this.setFormat('drawLabelsX', '--draw-labels-x', 'bool');
        this.setFormat('drawLabelsY', '--draw-labels-y', 'bool');
        
        this.setFormat('decimals', '--decimals', 'int');

        this.setFormat('min', '--min', 'float');
        this.setFormat('max', '--max', 'float');
    }

    getColor(i, v)
    {
        try
        {
            if(v && v>=0 && this.format.positiveColor)
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
                let l = this.format.color.split(",");
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
                let lw = this.parameters.labels ? parseInt(this.parameters.labelWidth) : 0;
                let r = this.canvasElement.getBoundingClientRect();
                let x = (evt.clientX - r.left - this.format.spaceLeft - lw)/(r.width - this.format.spaceLeft - this.format.spaceRight- lw);
                let y = (evt.clientY - r.top - this.format.spaceTop)/(r.height - this.format.spaceTop - this.format.spaceBottom);
                //this.get("/command/"+this.parameters.module+"/"+this.parameters.command+"/"+x+"/"+y+"/1");
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
        this.parentElement.className = this.parentElement.className.replace(/visible/,'');
        this.parentElement.className += this.toBool(this.parameters.show_frame) ? ' visible' : '';
        this.parentElement.firstChild.style.display = this.toBool(this.parameters.show_title) ? 'block' : 'none';
        this.parentElement.firstChild.innerText = this.parameters.title;
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
        //alert("parameterChangeNotification");
        this.updateAll();
        // interaction.setWidgetParameter(p); // FIXME: is this needed?
    }

    send_control_change(parameter, value=0, index_x=0, index_y=0)
    {
        controller.queueCommand("control", parameter, {"x":index_x, "y":index_y, "value":value});     
    }

    send_command(command, value=0, index_x=0, index_y=0)
    {
           //  this.get("/command/"+command+"/"+index_x+"/"+index_y+"/"+value);
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
