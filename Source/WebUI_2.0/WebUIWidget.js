class WebUIWidget extends HTMLElement
{
    // functions that should be overridden in subclasses
    
    init()
    {
    }
    
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

    constructor()
    {
        super();
        let pt = this.constructor.template();
        this.parameters = {};
        for(let i in pt)
            this.parameters[pt[i].name] = pt[i]['default'];
        this.parameter_template = pt;        
     }

   get(url, callback)
    {
        var last_request = url;

        xhr = new XMLHttpRequest();
        xhr.open("GET", "http://127.0.0.1:8000"+url, true);

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
        try
        {
            data_set.add(this.parameters['module']+"."+this.parameters['source']);
        }
        catch(err)
        {
        }
    }

    getProp(attribute, index)
    {
        try
        {
            if(index)
                return getComputedStyle(this).getPropertyValue(attribute).split(",")[index];
            else
                return getComputedStyle(this).getPropertyValue(attribute).trim();
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

    readCSSvariables()
    {

        // read CSS variables - TODO: allow to work also when some variable is missing

        this.format = {}
        this.format.titleHeight =           this.getInt('--title-height');
        this.format.titleFont =             this.getProp('--title-font');
        this.format.titleColor =            this.getProp('--title-color');
        this.format.titleBackground =       this.getProp('--title-background');
        this.format.titleMargins =          this.getBool('--title-margins');
        this.format.titleAlign =            this.getProp('--title-align');
        this.format.titleOffsetX =          this.getFloat('--title-offset', 0);
        this.format.titleOffsetY =          this.getFloat('--title-offset', 1);

        this.format.marginLeft =            this.getInt('--margin-left');
        this.format.marginRight =           this.getInt('--margin-right');
        this.format.marginTop =             this.getInt('--margin-top');
        this.format.marginBottom =          this.getInt('--margin-bottom');

        this.format.spaceLeft =             this.getInt('--space-left');
        this.format.spaceRight =            this.getInt('--space-right');
        this.format.spaceTop =              this.getInt('--space-top');
        this.format.spaceBottom =           this.getInt('--space-bottom');

        this.format.spacing =               this.getFloat('--spacing');

        this.format.color =                 this.getProp('--color');
        this.format.positiveColor =         this.getProp('--positive-color');
        this.format.negativeColor =         this.getProp('--negative-color');
        this.format.lineWidth =             this.getProp('--line-width');
        this.format.fill =                  this.getProp('--fill');

        this.format.gridColor =             this.getProp('--grid-color');
        this.format.gridLineWidth =         this.getProp('--grid-line-width');
        this.format.gridFill =              this.getProp('--grid-fill');

        this.format.frame =                 this.getProp('--frame');
        this.format.xAxis =                 this.getBool('--x-axis');
        this.format.yAxis =                 this.getBool('--y-axis');
        this.format.axisColor =             this.getProp('--axis-color');
        this.format.verticalGridlines =     this.getInt('--vertical-gridlines');
        this.format.horizontalGridlines =   this.getInt('--horizontal-gridlines');
        this.format.leftTickMarks =         this.getInt('--left-tick-marks');
        this.format.rightTickMarks =        this.getInt('--right-tick-marks');
        this.format.bottomTickMarks =       this.getInt('--bottom-tick-marks');
        this.format.leftScale =             this.getInt('--left-scale');
        this.format.rightScale =            this.getInt('--right-scale');
        this.format.bottomScale =           this.getInt('--bottom-scale');
        this.format.scaleOffset =           this.getInt('--scale-offset');
        this.format.scaleFont =             this.getProp('--scale-font');
        this.format.labels =                this.getBool('--labels');
        this.format.labelColor =            this.getProp('--label-color');
        this.format.labelFont =             this.getProp('--label-font');

        this.format.direction =             this.getProp('--direction');

        this.format.decimals =              this.getInt('--decimals');
        this.format.min =                   this.getInt('--min');
        this.format.max =                   this.getInt('--max');
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

    connectedCallback()
    {
        this.innerHTML = this.constructor.html();

        // These are for documentation purposes:
        
        this.onmousedown = function () { console.log("WebUIWidgetCanvas: mouse down"); }
        this.onmouseup = function () { console.log("WebUIWidgetCanvas: mouse up"); }
        this.onclick = function () { console.log("WebUIWidgetCanvas: click"); }
        this.onmousemove = function () { console.log("WebUIWidgetCanvas: mousemove"); }
        this.onmouseover = function () { console.log("WebUIWidgetCanvas: mouseover"); }
        this.onmouseout = function () { console.log("WebUIWidgetCanvas: mouseout"); }

        this.updateStyle(this, this.parameters['style']);
        this.updateStyle(this.parentNode, this.parameters['frame-style']);
        this.readCSSvariables();

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

