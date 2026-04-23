/*
 * New version of WebUI interface with BrainStudio functions
 */

const widget_classes = 
[
    "bar-graph",
    "plot",
    "table",
    "marker",
    "path",
    "grid",
    "image",
    "text",
    "rectangle",
    "button",
    "slider-horizontal",
    "slider-vertical",
    "switch",
    "drop-down-menu",
    "control-grid",
    "canvas3d",
    "epi-head",
    "key-points"
]

const identifier = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.0123456789";


function isEmpty(obj) 
{
    if(obj)
        for (const prop in obj) 
        {
            if (Object.hasOwn(obj, prop)) 
                return false;
        }
    return true;
}

function deepCopy(source) 
{
    if (source === null || typeof source !== 'object') 
      return source;
  
    if (Array.isArray(source)) {
      return source.map(item => deepCopy(item));
    }
  
    const newObject = {};
    for (const key in source) {
      newObject[key] = deepCopy(source[key]);
    }
    return newObject;
}


function replaceProperties(target, source) 
{
    // Remove all properties from the target object
    for (const key in target) {
      if (Object.hasOwnProperty.call(target, key)) {
        delete target[key];
      }
    }
  
    // Copy properties from the source object to the target object
    for (const key in source)
        if (Object.hasOwnProperty.call(source, key)) 
            target[key] = deepCopy(source[key]);
}
  
  function toggleStrings(array, toggleItems)
    {
        toggleItems.forEach(item => 
        {
            const index = array.indexOf(item);
            if (index === -1)
                array.push(item);
            else 
                array.splice(index, 1);
        });
  }


  function removeStringFromStart(mainString, stringToRemove)
  {
    if (mainString.startsWith(stringToRemove))
        return mainString.slice(stringToRemove.length);
    return mainString;
  }

function splitAtLastDot(str) {
    const lastDotIndex = str.lastIndexOf('.');
    if (lastDotIndex === -1) {
        return [str, ""]; // No dot found, return original string and empty string
    }
    
    const beforeDot = str.substring(0, lastDotIndex);
    const afterDot = str.substring(lastDotIndex + 1); // +1 to skip the dot
    return [beforeDot, afterDot];
}



function toURLParams(params) {
    return Object.keys(params).map((key) => {
        //console.log(key);
        return encodeURIComponent(key) + '=' + encodeURIComponent(params[key]);
    }).join('&');
}

const inspectorUIColorOptions = "red,orange,yellow,green,blue,purple,pink,white,black";
const logLevelOptions = ["inherit", "quiet", "exception", "end_of_file", "terminate", "fatal_error", "warning", "print", "debug", "trace"];
const inspectorUIColorLabels = {
    red: "&#x1F7E5; red",
    orange: "&#x1F7E7; orange",
    yellow: "&#x1F7E8; yellow",
    green: "&#x1F7E9; green",
    blue: "&#x1F7E6; blue",
    purple: "&#x1F7EA; purple",
    pink: "pink",
    white: "&#x2B1C; white",
    black: "&#x2B1B; black"
};
const inspectorUIColorStyles = {
    red: "background:#d84a4a;color:#fff;",
    orange: "background:#e58e3a;color:#111;",
    yellow: "background:#efe26a;color:#111;",
    green: "background:#58a663;color:#fff;",
    blue: "background:#4f79c8;color:#fff;",
    purple: "background:#7f59b3;color:#fff;",
    pink: "background:#d77db2;color:#111;",
    white: "background:#f5f5f5;color:#111;",
    black: "background:#1a1a1a;color:#fff;"
};
const inspectorUIColorSwatches = {
    red: "#d84a4a",
    orange: "#e58e3a",
    yellow: "#efe26a",
    green: "#58a663",
    blue: "#4f79c8",
    purple: "#7f59b3",
    pink: "#d77db2",
    white: "#f5f5f5",
    black: "#1a1a1a"
};

function parseInspectorRGBColor(colorValue)
{
    const fallback = { r: 0, g: 0, b: 0 };
    if(colorValue === undefined || colorValue === null)
        return fallback;
    const rawValue = String(colorValue).trim().toLowerCase();
    const namedValue = inspectorUIColorSwatches[rawValue];
    const normalized = (namedValue || rawValue).replace(/^#/, "");
    if(!/^[0-9a-fA-F]{6}$/.test(normalized))
        return fallback;
    return {
        r: parseInt(normalized.slice(0, 2), 16),
        g: parseInt(normalized.slice(2, 4), 16),
        b: parseInt(normalized.slice(4, 6), 16)
    };
}

function formatInspectorRGBColor(r, g, b)
{
    const toHex = function(value)
    {
        const n = Math.max(0, Math.min(255, parseInt(value, 10) || 0));
        return n.toString(16).padStart(2, "0").toUpperCase();
    };
    return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
}

function clampInspectorColorChannel(value)
{
    return Math.max(0, Math.min(255, Math.round(value)));
}

function mixInspectorColorChannel(a, b, ratio)
{
    return clampInspectorColorChannel(a + (b - a) * ratio);
}

function buildInspectorDerivedPalette(colorValue)
{
    const { r, g, b } = parseInspectorRGBColor(colorValue);
    const luminance = (0.2126 * r + 0.7152 * g + 0.0722 * b) / 255;
    const mix = function(target, ratio)
    {
        return formatInspectorRGBColor(
            mixInspectorColorChannel(r, target.r, ratio),
            mixInspectorColorChannel(g, target.g, ratio),
            mixInspectorColorChannel(b, target.b, ratio)
        );
    };
    const darkTarget = { r: 0, g: 0, b: 0 };
    const lightTarget = { r: 255, g: 255, b: 255 };
    return {
        bg: mix(darkTarget, 0.58),
        titleBg: mix(darkTarget, 0.42),
        rowBg: mix(lightTarget, 0.18),
        classBg: mix(darkTarget, 0.32),
        separator: mix(lightTarget, 0.28),
        border: luminance > 0.65 ? "#333333" : "",
        titleFg: luminance > 0.6 ? "#222222" : "#f7f7f7",
        rowFg: luminance > 0.7 ? "#2b2b2b" : "#f1f1f1"
    };
}


function setType(x, t)
{

    if(t === 'int')
        return parseInt(x);
    
    if(t === 'float')
        return parseFloat(x);
    
    if(t === 'bool')
        return ['on','yes','true'].includes(x.toString().toLowerCase());
    
    return x;
};

function zeroPad(x)
{
    if(x <10)
        return "0"+x;
    else
        return String(x);
}

function secondsToHMS(d)
{
    try
    {
        if(d=="-")
            return "-";

        d = Number(d);
        if(isNaN(d) || d < 0)
            return "-";

        const h = Math.floor(d / 3600);
        const m = Math.floor(d % 3600 / 60);
        const s = Math.floor(d % 3600 % 60);
        return h+":"+zeroPad(m)+":"+zeroPad(s);
    }
    catch(err)
    {
        return "-";
    }
}

function formatTime(d)
{
    try
    {
        if(d == "-")
            return "-";
        if(d < 0)
            return "-";
        else if(d < 0.001)
            return Math.round(1000000*d) + " &#181;s";
        else if(d < 5)
            return Math.round(1000*d) + " ms";
        else
            return secondsToHMS(d);
    }
    catch(e)
    {
        return "-";
    }
}

function changeNameInPath(path, name)
{
    const ix = path.lastIndexOf('.');
    if (ix === -1)
        return name;
    return path.substring(0, ix + 1) + name;
}


function nameInPath(path)
{
    const ix = path.lastIndexOf('.');
    if (ix === -1)
        return "";
    return path.substring(ix + 1);
}

function parentPath(path)
{
    const p = path.split('.');
    p.pop();
    return p.join('.');
    
}

function getStringUpToBracket(str)
{
    const index = str.indexOf('[');
    if (index === -1) {
      return str;
    }
    return str.substring(0, index);
}


function getStringAfterBracket(str)
{
    const index = str.indexOf('[');
    if (index === -1) {
      return "";
    }
    return str.substring(index, str.length);
}

function parseBracketRangeString(str)
{
    const source = typeof str === "string" ? str : "";
    const matches = source.match(/\[[^\]]*\]/g);
    if(!matches)
        return [];

    return matches.map((token) =>
    {
        const content = token.slice(1, -1);
        const parts = content.split(':');
        return {
            start: parts[0] ?? "",
            end: parts.length > 1 ? (parts[1] ?? "") : "",
            step: parts.length > 2 ? parts.slice(2).join(':') : ""
        };
    });
}

function serializeBracketRangeParts(parts)
{
    if(!Array.isArray(parts) || parts.length === 0)
        return "";

    return parts.map((part) =>
    {
        const start = part && part.start != null ? String(part.start).trim() : "";
        const end = part && part.end != null ? String(part.end).trim() : "";
        const step = part && part.step != null ? String(part.step).trim() : "";

        if(end === "" && step === "")
            return `[${start}]`;
        if(step === "")
            return `[${start}:${end}]`;
        return `[${start}:${end}:${step}]`;
    }).join("");
}

function setCookie(name,value,days=100)
{
    let date = new Date();
    date.setTime(date.getTime()+(days?days:1)*86400000);
    let expires = "; expires="+date.toGMTString();
    document.cookie = name+"="+value+expires+"; path=/";
}

function getCookie(name)
{
    let nameEQ = name + "=";
    const ca = document.cookie.split(';');
    for(let i=0;i < ca.length;i++) {
        let c = ca[i];
        while (c.charAt(0)==' ') c = c.substring(1,c.length);
        if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
    }
    return null;
}

function eraseCookie(name)
{
    createCookie(name,"",-1);
}

function resetCookies()
{
    setCookie('selected_background', "");
}
const network = 
{
    network: null,
    classes: null,
    classinfo: {},
    dict: {},
    tainted: false,
    component_count: 0,

    init(n)
    {
        this.network = n;
        this.ensureGroupAutoRouting(this.network);
        this.ensureGroupGridSpacing(this.network);
        this.ensureConnectionDefaults(this.network);
        this.rebuildDict();
        this.component_count = Object.keys(this.dict).length+1;
    },

    hasAnyComponentPosition(group=this.network)
    {
        if(!group || typeof group !== "object")
            return false;

        const hasPosition = function(component)
        {
            if(!component || typeof component !== "object")
                return false;
            const x = parseFloat(component._x);
            const y = parseFloat(component._y);
            return Number.isFinite(x) && Number.isFinite(y);
        };

        const components = [
            ...(group.groups || []),
            ...(group.modules || []),
            ...(group.inputs || []),
            ...(group.outputs || []),
            ...(group.widgets || [])
        ];

        for(const component of components)
        {
            if(hasPosition(component))
                return true;
            if(component && component._tag === "group" && this.hasAnyComponentPosition(component))
                return true;
        }

        return false;
    },

    ensureGroupAutoRouting(group)
    {
        if(!group || typeof group !== "object")
            return;
        if(group.auto_routing === undefined)
            group.auto_routing = false;
        for(const child of group.groups || [])
            this.ensureGroupAutoRouting(child);
    },

    ensureGroupGridSpacing(group)
    {
        if(!group || typeof group !== "object")
            return;
        const parsed = parseInt(group.grid_spacing, 10);
        if(Number.isFinite(parsed))
            group.grid_spacing = Math.min(192, Math.max(8, parsed));
        else
            group.grid_spacing = 24;
        for(const child of group.groups || [])
            this.ensureGroupGridSpacing(child);
    },

    ensureConnectionDefaults(group)
    {
        if(!group || typeof group !== "object")
            return;

        for(const connection of group.connections || [])
        {
            if(!connection || typeof connection !== "object")
                continue;
            if(connection.line_type === undefined || connection.line_type === null || connection.line_type === "")
                connection.line_type = "auto_route";
        }

        for(const child of group.groups || [])
            this.ensureConnectionDefaults(child);
    },

    isUnique(name)  // test if name can be changed to this ******************************
    {
        return true;
    },

    uniqueID(name)
    {
        let bg = selector.selected_background;
        let i = 1;
        test_name = bg+'.'+name+i;
        while(test_name in network.dict)
            test_name = bg+'.'+name+(++i);
        return name+i;
    },

    rebuildDict()
    {
        this.dict = {};
        this.buildDict(this.network, "");
    },

 

    addComponentsToDict(path, components)
    {
        for(let c of components || [])
            this.dict[path+'.'+c.name] = c;
    },


    buildDict(o, path)
    {
        let new_path = (path+'.'+o.name); //.substring(1);
        if(new_path[0] === '.')
            new_path = new_path.substring(1);

        network.dict[new_path] = o;

        for(let g of o.groups || [])
            this.buildDict(g, new_path)

        for(let c of o.connections || [])
            this.dict[new_path+'.'+getStringUpToBracket(c.source)+'*'+new_path+'.'+getStringUpToBracket(c.target)] = c;

        this.addComponentsToDict(new_path, o.inputs);
        this.addComponentsToDict(new_path, o.outputs);
        this.addComponentsToDict(new_path, o.modules);
        this.addComponentsToDict(new_path, o.widgets);
    },



    newConnection(path, source, target)
    {
        const connection = 
        {
            _tag: "connection",
            source: source,
            target: target,
            delay: "1",
            color: "black",
            line_type: "auto_route"
        };
        let group = network.dict[path];
        if(group.connections == null)
            group.connections = [];
        group.connections.push(connection);
        network.dict[path+"."+source+"*"+path+"."+target]=connection;
    },


    getParentGroup(fullComponentName)
    {

    },


    pruneConnections()
    {
        // Handled in kernel
    },


    changeModuleClass(module, new_class)
    {
        let old_module = deepCopy(network.dict[module]);
        replaceProperties(network.dict[module], network.classinfo[new_class]);
        let new_module = network.dict[module];
        new_module._tag = "module";
        new_module.class = new_class;
        new_module.name = old_module.name;
        new_module._x = old_module._x;
        new_module._y = old_module._y;
        new_module.color = old_module.color;
        // TODO: Check that all properties are in the class
        network.dict[module] = new_module;
        // FIXME: Update existing connections if possible
    },

    changeWidgetClass(widget, new_class)
    {
        const old_widget = deepCopy(network.dict[widget]);
        let new_widget  = 
        {
            _tag: "widget",
            name: old_widget.name,
            title: old_widget.title,
            class: new_class,
            _x:old_widget._x,
            _y:old_widget._y,
            width: old_widget.width,
            height: old_widget.height,
            show_title: old_widget.show_title,
            show_frame: old_widget.show_frame,

            source: old_widget.source || "",
            min: (old_widget.min !== undefined ?  old_widget.min : ""),
            max: (old_widget.max !== undefined ?  old_widget.max : ""),
            select: old_widget.select || "",
        };


        replaceProperties(network.dict[widget], new_widget);
    },


    renameGroupOrModule(group, old_name, new_name)
    {
        network.dict[new_name] = network.dict[old_name];
        delete network.dict[old_name];

        // Update connections to and from this group or module
        let parent_path = parentPath(old_name);
        let parent_group = network.dict[group];
        let connection_parent = "";
        for(let c of parent_group.connections || [])
        {
            // Update if source (without output) == old_name
            connection_parent = parentPath(c.source);
            if(connection_parent == nameInPath(old_name))
            {
                let p = c.source.split('.');
                c.source = nameInPath(new_name)+'.'+p[1];
            }
            // Update of target == this
            connection_parent = parentPath(c.target);
            if(connection_parent == nameInPath(old_name))
            {
                let p = c.target.split('.');
                c.target = nameInPath(new_name)+'.'+p[1];
            }
        }
    },  

    renameInput(group, old_name, new_name) // FIXME: NEW PATH not NAME
    {
        for(let c of network.dict[group].connections || [])
        {
            if(group+'.'+c.source == old_name)
            {
                c.source = nameInPath(new_name); // FIXME: USE END OF NEW_NAME - changeNameInPath(selector.selected_background, inspector.item.name);
            }
        }
        let parent_path = parentPath(group);
        let parent_group = network.dict[parent_path];

        if(parent_group)
            for(let c of parent_group.connections || [])
            if(parent_path+'.'+c.target == old_name)
            {
                c.target = changeNameInPath(c.target, inspector.item.name);
            }
            network.rebuildDict();
    },

    renameOutput(group, old_name, new_name)
    {
        for(let c of network.dict[group].connections || [])
        {
            if(group+'.'+c.target == old_name)
            {
                c.target = nameInPath(new_name);
            }
        }
        let parent_path = parentPath(group);
        let parent_group = network.dict[parent_path];

        if(parent_group)
            for(let c of parent_group.connections || [])
            if(parent_path+'.'+c.source == old_name)
            {
                c.source = changeNameInPath(c.source, inspector.item.name);
            }
        network.rebuildDict();
        nav.populate();
    },

    debug_json()
    {
        let w = window.open("about:blank", "", "_blank");
        w.document.write('<html><body><pre>'+JSON.stringify(network.network,null,2)+ '</pre></body></html>');
    },

    debug_dict()
    {
        let w = window.open("about:blank", "", "_blank");
        w.document.write('<html><body><h1>All components (in network.dict)</h1><pre>');
        w.document.write('<html><body><pre>');
        for(let k of Object.keys(network.dict))
            w.document.write(k+"\n");
        w.document.write('</pre></body></html>');
    }
}

const webui_widgets =
{
    constructors: {},
    add: function(element_name, class_object) {
        customElements.define(element_name, class_object);
        webui_widgets.constructors[element_name] = class_object;
    }
};

/*
 *
 * Selector     -       select in 'navigator', 'breadcrums', 'inspector' and 'main'
 * 
 */

const brainstudio = 
{
    keydownHandler(evt)
    {
        const key = (evt.key || "").toLowerCase();
        const code = evt.code || "";
        const isModifier = evt.metaKey || evt.ctrlKey;
        const infoDialog = document.getElementById("info_dialog");

        if(infoDialog && infoDialog.open && evt.key === "Escape")
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            dialog.closeInfo();
            return;
        }

        if(isModifier && evt.altKey && (key === "t" || code === "KeyT"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.realtime();
            return;
        }

        if(isModifier && evt.altKey && (key === "p" || code === "KeyP"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.play();
            return;
        }

        if(isModifier && evt.altKey && (key === "." || code === "Period"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.stop();
            return;
        }

        // Use Cmd/Ctrl+Option+O for Open to avoid browser-reserved Cmd+O.
        if(isModifier && evt.altKey && (key === "o" || code === "KeyO"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.open();
            return;
        }

        if(isModifier && evt.altKey && (key === "n" || code === "KeyN"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.new();
            return;
        }

        main.keydown(evt);
    },

    init()
    {
        log.init();
        inspector.init();
        nav.init();
        breadcrumbs.init();
        main.init();
        controller.init();

        window.addEventListener("keydown", brainstudio.keydownHandler, true);

    }
}
