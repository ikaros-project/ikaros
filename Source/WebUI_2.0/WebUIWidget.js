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
            return ['on','yes','true'].includes(x); // .toLowerCase()
        
        return x;
    }

    constructor()
    {
        super();
        let pt = this.constructor.template();
        this.parameters = {};
        for(let i in pt)
            this.parameters[pt[i].name] = this.setType(pt[i]['default'], pt[i]['type']);
        this.parameter_template = pt;        
     }

   get(url, callback)
    {
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

    requestData(data_set)
    {
        if(this.parameters['module'] && this.parameters['source'])
            data_set.add(this.parameters['module']+"."+this.parameters['source']);
    }

    // ACCESS FUNCTIONS: Need some cleanup

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

    getBool(attribute, index)
    {
        try
        {
            if(index)
                return ['yes','true'].includes(this.getProp(attribute).split(",")[index].toLowerCase());
            else
                return ['yes','true'].includes(this.getProp(attribute).toLowerCase());
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
        if(variable in this.parameters)
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
        // read CSS variables - TODO: allow to work also when some variable is missing

        this.format = {}

        this.setFormat('direction', '--direction', 'string');

        this.setFormat('titleHeight', '--title-height', 'int');
        this.setFormat('titleFont', '--title-font', 'string');
        this.setFormat('titleColor', '--title-color', 'string');
        this.setFormat('titleBackground', '--title-background', 'string');
        this.setFormat('titleMargins', '--title-margins','int');
        this.setFormat('titleAlign', '--title-align', 'string');
        this.setFormat('titleOffsetX', '--title-offset','int', 0);
        this.setFormat('titleOffsetY', '--title-offset','int', 1);

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
        this.setFormat('fill', '--fill', 'string');

        this.setFormat('gridColor', '--grid-color', 'string');
        this.setFormat('gridLineWidth', '--grid-line-width', 'string');
        this.setFormat('gridFill', '--grid-fill', 'string');

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
        this.setFormat('min', '--min', 'int');
        this.setFormat('max', '--max', 'int');
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
        this.parentElement.className += this.parameters.show_frame ? ' visible' : '';
        this.parentElement.firstChild.style.display = this.parameters.show_title ? 'block' : 'none';
        this.parentElement.firstChild.innerText = this.parameters.title;
        this.setCSSClass();
        this.readCSSvariables(); // TEST
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
};

customElements.define('webui-widget', WebUIWidget);
