"don't use strict";

/*
 *  Extensions of string
 */

String.prototype.rsplit = function(sep, maxsplit) {
    var split = this.split(sep || /\s+/);
    return maxsplit ? [ split.slice(0, -maxsplit).join(sep) ].concat(split.slice(-maxsplit)) : split;
}


// COOKIES FOR PERSISTENT STATE

function setCookie(name,value,days=100)
{
    var date = new Date();
    date.setTime(date.getTime()+(days?days:1)*86400000);
    var expires = "; expires="+date.toGMTString();
    document.cookie = name+"="+value+expires+"; path=/";
}

function getCookie(name)
{
    var nameEQ = name + "=";
    var ca = document.cookie.split(';');
    for(var i=0;i < ca.length;i++) {
        var c = ca[i];
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
    setCookie('current_view', "");
//    setCookie('root', ""); // or /
//    setCookie('inspector',"closed");
}



/*
 *
 * Viewer scripts
 *
 */

function test_sleep(millis)
{
    var date = new Date();
    var curDate = null;
    do { curDate = new Date(); }
    while(curDate-date < millis);
}

function copyToClipboard(text)
{
    if (document.queryCommandSupported && document.queryCommandSupported("copy")) {
        var textarea = document.createElement("textarea");
        textarea.textContent = text;
        textarea.style.position = "fixed";
        document.body.appendChild(textarea);
        textarea.select();
        try {
            return document.execCommand("copy");  // Security exception may be thrown by some browsers.
        } catch (ex) {
            console.warn("Copy to clipboard failed.", ex);
            return false;
        } finally {
            document.body.removeChild(textarea);
        }
    }
}

function toggleNav()
{
    var x = document.getElementById('navigator');
    var s = window.getComputedStyle(x, null);
    if (s.display === 'none') {
        x.style.display = 'block';
    } else {
        x.style.display = 'none';
    }
}

function toggleFooter()
{
    var x = document.querySelector('footer');
    var s = window.getComputedStyle(x, null);
    if (s.display === 'none') {
        x.style.display = 'block';
    } else {
        x.style.display = 'none';
    }
}

function toggleInspector()
{
    var x = document.getElementById('widget_inspector');
    var s = window.getComputedStyle(x, null);
    if (s.display === 'none') {
        x.style.display = 'block';
    } else {
        x.style.display = 'none';
    }
}

function toggleModuleInspector()
{
    var x = document.getElementById('module_inspector');
    var s = window.getComputedStyle(x, null);
    if (s.display === 'none') {
        x.style.display = 'block';
    } else {
        x.style.display = 'none';
    }
}

function toggleSystem()
{
    var x = document.getElementById('system_inspector');
    var s = window.getComputedStyle(x, null);
    if (s.display === 'none') {
        x.style.display = 'block';
    } else {
        x.style.display = 'none';
    }
}


/*
 *
 * Navigator scripts
 *
 */

nav = {

    init: function (g) {
        nav.group = g;
        nav.navigator = document.getElementById('navigator');
        nav.populate(nav.navigator);
//        nav.navigator.addEventListener("click", nav.navClick, false);
    },
    toggleGroup(e) {
        if(e.target.getAttribute("class") == "group-open")
            e.target.setAttribute("class", "group-closed");
        else if(e.target.getAttribute("class") == "group-closed")
            e.target.setAttribute("class", "group-open");
        e.stopPropagation();
    },
    openGroup(item) {
        let g = nav.navigator.querySelector("[data-name='"+item+"']");
        g = g.parentElement;
        while(g)
        {
            g.setAttribute("class", "group-open");
            g = g.parentElement;
        }
    },
    selectItem(item) {
        interaction.addView(item)
    },
    selectModule(evt)
    {
    
    },
    navClick: function(e) {
        nav.selectItem(e.target.parentElement.dataset.name);
        e.stopPropagation();
    },
    buildList: function(group, name) {
        let s = "";
//        if(group.groups.length + group.views.length == 0)
//            s = "<li data-name='"+name+"/"+group.attributes.name+"'  class='XXX'><span onclick='return nav.navClick(event)'>" + group.attributes.name + "</span>"; // FIXME: or title
//        else
            s = "<li data-name='"+name+"/"+group.attributes.name+"'  class='group-closed' onclick='return nav.toggleGroup(event)'><span onclick='return nav.navClick(event)'>" + group.attributes.name + "</span>"; // FIXME: or title
        if(group.views)
        {
            s +=  "<ul>"
            for(i in group.views)
            {
                if(!group.views[i].name)
                    group.views[i].name = "View #"+i;
                s += "<li data-name='"+name+"/"+group.attributes.name+"#"+group.views[i].name+"'>-&nbsp" + "<span  onclick='return nav.navClick(event)'>"+ group.views[i].name + "</span></li>";
            }
            s += "</ul>";
        }
        if(group.groups)
        {
            s +=  "<ul>"
            for(i in group.groups)
                s += nav.buildList(group.groups[i], name+"/"+group.attributes.name);
            s += "</ul>";
        }
        s += "</li>";
        return s;
    },
    populate: function (element) {
        element.innerHTML = "<ul>"+nav.buildList(nav.group, "")+"</ul>";
    }
}


/*
 *
 * Inspector scripts
 *
 */
inspector = {
    inspector: null,
    table: null,
    list: null,
    webui_object: null,
    
    init: function () {
        inspector.inspector = document.getElementById('widget_inspector');
        inspector.table = document.getElementById('i_table');
    },
    remove: function () {
        while(inspector.table.rows.length)
            inspector.table.deleteRow(-1);
    },
    add: function (webui_object) {
        let widget = webui_object.widget;
        let parameters = widget.parameters;

        inspector.webui_object = webui_object;
        inspector.parameter_template = widget.parameter_template;

        for(let p of inspector.parameter_template)
        {
            let row = inspector.table.insertRow(-1);
            let value = parameters[p.name];
            let cell1 = row.insertCell(0);
            let cell2 = row.insertCell(1);
            cell1.innerText = p.name;
            cell2.innerHTML = value;
            cell2.setAttribute('class', p.type);
            switch(p.control)
            {
                case 'header':
                    cell1.setAttribute("colspan", 2);
                    cell1.setAttribute("class", "header");
                    row.deleteCell(1);
                    break;

                case 'textedit':
                    cell2.contentEditable = true;
                    cell2.className += ' textedit';
                    cell2.addEventListener("keypress", function(evt) {
                        if(evt.keyCode == 13)
                        {
                            evt.target.blur();
                            evt.preventDefault();
                            return;
                        }
                        if(p.type == 'int' && "-0123456789".indexOf(evt.key) == -1)
                            evt.preventDefault();
                        else if(p.type == 'float' && "-0123456789.".indexOf(evt.key) == -1)
                            evt.preventDefault();
                        else if(p.type == 'source' && "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-_.0123456789*".indexOf(evt.key) == -1)
                            evt.preventDefault();
                    });
                    cell2.addEventListener("blur", function(evt) {
                        if(p.type == 'int')
                            parameters[p.name] = parseInt(evt.target.innerText);
                        else if(p.type == 'float')
                            parameters[p.name] = parseFloat(evt.target.innerText);
                        else
                        {
                            parameters[p.name] = evt.target.innerText.replace(String.fromCharCode(10), "").replace(String.fromCharCode(13), "");
                            if(p.name == "style")
                                widget.updateStyle(widget, evt.target.innerText);
                            if(p.name == "frame-style")
                                widget.updateStyle(webui_object, evt.target.innerText);
                         }
                        widget.updateAll();
                    });
                break;

                case 'slider':
                    if(p.type == 'int' || p.type == 'float')
                    {
                        cell2.innerHTML= '<div>'+value+'</div><input type="range" value="'+value+'" min="'+p.min+'" max="'+p.max+'" step="'+(p.type == 'int' ?  1: 0.01)+'"/>';
                        cell2.addEventListener("input", function(evt) {
                            evt.target.parentElement.querySelector('div').innerText = evt.target.value;
                            parameters[p.name] = evt.target.value;
                            widget.updateAll();
                        });
                    }
                break;
                
                case 'menu':
                    var opts = p.values.split(',').map(o=>o.trim());
                    
                    var s = '<select name="'+p.name+'">';
                    for(var j in opts)
                    {
                        let value = p.type == 'int' ? j : opts[j];
                        if(opts[j] == parameters[p.name])
                            s += '<option value="'+value+'" selected >'+opts[j]+'</option>';
                        else
                            s += '<option value="'+value+'">'+opts[j]+'</option>';
                    }
                    s += '</select>';
                    cell2.innerHTML= s;
                    cell2.addEventListener("input", function(evt) { parameters[p.name] = evt.target.value.trim(); widget.updateAll();});
                break;
                
                case 'checkbox':
                    if(p.type == 'bool')
                    {
                        if(value)
                            cell2.innerHTML= '<input type="checkbox" checked />';
                        else
                            cell2.innerHTML= '<input type="checkbox" />';
                        cell2.addEventListener("change", function(evt) { parameters[p.name] = evt.target.checked; widget.updateAll();});
                    }
                break;
                
                case 'number':
                    if(p.type == 'int')
                    {
                        cell2.innerHTML= '<input type="number" value="'+value+'" min="'+p.min+'" max="'+p.max+'"/>';
                        cell2.addEventListener("input", function(evt) { parameters[p.name] = evt.target.value; widget.updateAll();});
                    }
                break;
                
                default:
                
                break;
            }
        }
    },
    select: function (obj)
    {
        inspector.remove();
        inspector.add(obj);
    },
    update: function (attr_value)
    {
        // New data from server
    },
    change: function (attr_value)
    {
        // Send changed value to server
    }
}



/*
 *
 * Module inspector scripts
 *
 */
module_inspector = {
    inspector: null,
    table: null,
    list: null,
    webui_object: null,
    
    init: function () {
        module_inspector.inspector = document.getElementById('module_inspector');
        module_inspector.table = document.getElementById('mi_table');
    },
    remove: function () {
        while(module_inspector.table.rows.length)
            module_inspector.table.deleteRow(-1);
    },
    addHeader(title) {
        row = module_inspector.table.insertRow(-1);
        cell = row.insertCell(0);
        cell.innerText = title;
        cell.setAttribute("colspan", 2);
        cell.setAttribute("class", "header");
    },
    addRow(attribute, value) {
        value = value!=undefined ? value : "";
        row = module_inspector.table.insertRow(-1);
        cell1 = row.insertCell(0);
        cell2 = row.insertCell(1);
        cell1.innerText = attribute;
        cell2.innerHTML = value;
    },
    add: function (module) {
    //    let widget = webui_object.widget;
    //    let parameters = widget.parameters;

        module_inspector.module = module;
   //     module_inspector.parameter_template = widget.parameter_template;

        // Add header info

        let m = module_inspector.module;
        
        if(m.parameters.groups.length > 0) // add group
        {
            module_inspector.addHeader("GROUP");
            module_inspector.addRow("name", m.parameters.attributes.name);
            for(let p in m.parameters.parameters)
                if(p != "parameters" && p != "views" && p!= "groups" && p!= "connections" && p!= "name" && p[0] != "_")
                {
                    let row = module_inspector.table.insertRow(-1);
                    let value = m.parameters.attributes[p];
                    module_inspector.addRow(p.name, m.parameters.attributes[p.name] ? m.parameters.attributes[p.name].toString() : p["default"]);
                }

            module_inspector.addHeader("SUBGROUPS");
            module_inspector.addRow("modules", m.parameters.groups.length);
            module_inspector.addRow("connections", m.parameters.connections.length);

            module_inspector.addHeader("APPEARANCE");
            module_inspector.addRow("x", m.parameters.attributes._x);
            module_inspector.addRow("y", m.parameters.attributes._y);
            module_inspector.addRow("color", m.parameters.attributes._color);
            module_inspector.addRow("text_color", m.parameters.attributes._text_color);
            module_inspector.addRow("shape", m.parameters.attributes._shape);
        }
        
        else // add module
        {
            module_inspector.addHeader("MODULE");
            module_inspector.addRow("name", m.parameters.attributes.name);
            module_inspector.addRow("class", m.parameters.attributes.class);

            for(let p of m.parameters.parameters)
                module_inspector.addRow(p.name, m.parameters.attributes[p.name] ? m.parameters.attributes[p.name].toString() : p["default"]);

            module_inspector.addRow("description", m.parameters.attributes.description);

            module_inspector.addHeader("APPEARANCE");
            module_inspector.addRow("x", m.parameters.attributes._x);
            module_inspector.addRow("y", m.parameters.attributes._y);
            module_inspector.addRow("color", m.parameters.attributes._color);
            module_inspector.addRow("text_color", m.parameters.attributes._text_color);
            module_inspector.addRow("shape", m.parameters.attributes._shape);
        }
    },
    select: function (obj)
    {
        module_inspector.remove();
        module_inspector.add(obj);
    },
    update: function (attr_value)
    {
        // New data from server
    },
    change: function (attr_value)
    {
        // Send changed value to server
    }
}


webui_widgets = {
    constructors: {},
    add: function(element_name, class_object) {
        customElements.define(element_name, class_object);
        webui_widgets.constructors[element_name] = class_object;
    }
};


/*
 *
 * Interaction scripts for Main Area
 *
 */

interaction = {
    initialMouseX: undefined,
    initialMouseY: undefined,
    startX: undefined,
    startY: undefined,
    selectedObject: undefined,
    grid_spacing: 20,
    sizegrid: 20,
    curnewpos: 20,
    editMode: true,
    main: undefined,
    currentView: undefined,
    currentViewName: undefined,
    widget_inspector: undefined,
    system_inspector: undefined,
    edit_inspector: undefined,
    module_inspector: undefined,
    network_inspector: undefined,

    init: function () {
        interaction.main = document.querySelector('main');
        interaction.widget_inspector = document.querySelector('#widget_inspector');
        interaction.system_inspector = document.querySelector('#system_inspector');
        interaction.edit_inspector = document.querySelector('#edit_inspector');
        interaction.module_inspector = document.querySelector('#module_inspector');
        interaction.network_inspector = document.querySelector('#network_inspector');
        interaction.setMode('run');
    },
    stopEvents: function (e) {
        if(interaction.main.dataset.mode == "edit") e.stopPropagation()
    },

    initDraggables: function () { // only needed if there are already frame elements in the main view
        let nodes = document.querySelectorAll(".frame");
        for (var i = 0; i <    nodes.length; i++)
            interaction.initElement(nodes[i]);
        let  main = document.querySelector('main');
        main.addEventListener('mousedown',interaction.deselectObject,false);
    },
    removeAllObjects() {
        let main = document.querySelector('main');
        let nodes = document.querySelectorAll(".frame");
        for (var i = 0; i < nodes.length; i++)
            main.removeChild(nodes[i]);

        nodes = document.querySelectorAll(".module");
        for (var i = 0; i < nodes.length; i++)
            main.removeChild(nodes[i]);
    },
    generateGrid: function (spacing) {
        interaction.grid_spacing = spacing;
        let grid = interaction.main.querySelector('#grid');
        if(grid)
            interaction.main.removeChild(grid);
        interaction.main.innerHTML += '<div id="grid"></div>'
        grid = document.getElementById('grid');
        for(let i=1; i<250; i++)
        {
            if(i*interaction.grid_spacing > 2000) // should check main canvas size instead
                break;
            grid.innerHTML += '<div class="vgrid" style="left:'+i*interaction.grid_spacing+'px; height: 2000px"></div>'
            grid.innerHTML += '<div class="hgrid" style="top:'+i*interaction.grid_spacing+'px; width: 2000px"></div>'
        }
    },
    changeGrid: function(spacing) {
        interaction.grid_spacing = spacing;
        vgrids = document.querySelectorAll('.vgrid');
        for(let i = 0; i < vgrids.length; i++)
            vgrids[i].style.left = ""+(i+1)*spacing+"px";
        hgrids = document.querySelectorAll('.hgrid');
        for(let i = 0; i < hgrids.length; i++)
            hgrids[i].style.top = ""+(i+1)*spacing+"px";
    },
    increaseGrid() {
        if(interaction.grid_spacing < 160)
            interaction.changeGrid(2*interaction.grid_spacing);
    },
    decreaseGrid() {
        if(interaction.grid_spacing > 10)
            interaction.changeGrid(0.5*interaction.grid_spacing);
    },
    drawArrow(context, arrow)
    {
        context.beginPath();
        context.moveTo(arrow[arrow.length-1][0],arrow[arrow.length-1][1]);
        for(var i=0;i<arrow.length;i++){
            context.lineTo(arrow[i][0],arrow[i][1]);
        }
        context.closePath();
        context.fill();
        context.stroke();
    },
    moveArrow(arrow, x, y)
    {
        var rv = [];
        for(var i=0;i<arrow.length;i++){
            rv.push([arrow[i][0]+x, arrow[i][1]+y]);
        }
        return rv;
    },
    rotateArrow(arrow,angle)
    {
        var rv = [];
        for(var i=0; i<arrow.length;i++){
            rv.push([(arrow[i][0] * Math.cos(angle)) - (arrow[i][1] * Math.sin(angle)),
                     (arrow[i][0] * Math.sin(angle)) + (arrow[i][1] * Math.cos(angle))]);
        }
        return rv;
    },
    drawArrowHead(context, fromX, fromY, toX, toY)
    {
        var angle = Math.atan2(toY-fromY, toX-fromX);
        var arrow = [[0,0], [-10,-5], [-10, 5]];
        context.save();
        context.lineJoin = "miter";
//        context.fillStyle = "black";
        this.drawArrow(context, this.moveArrow(this.rotateArrow(arrow,angle),toX,toY));
        context.restore();
    },
    drawConnections()
    {
        function bezier(t, p0, p1, p2, p3)
        {
            t2 = t * t;
            t3 = t2 * t;
            mt = 1-t;
            mt2 = mt * mt;
            mt3 = mt2 * mt;
            x = p0.x*mt3 + 3*p1.x*mt2*t + 3*p2.x*mt*t2 + p3.x*t3;
            y = p0.y*mt3 + 3*p1.y*mt2*t + 3*p2.y*mt*t2 + p3.y*t3;
            return {x:x, y:y};
        }

        function draw_chord(context, x0, y0, x1, y1, xc, yc, r)
        {
            let d = Math.hypot(x0-x1, y0-y1);
            let a = 0.3 + 0.5*d/(2*r);
            let b = 1-a;

            context.beginPath();
            context.moveTo(x0, y0);
            context.bezierCurveTo(a*xc+b*x0, a*yc+b*y0, a*xc+b*x1, a*yc+b*y1, x1, y1);
            context.stroke();
        }

        function draw_bez_arrow(context, x0, y0, x1, y1, xc, yc, r)
        {
            let d = Math.hypot(x0-x1, y0-y1);
            let a = 0.3 + 0.5*d/(2*r);
            let b = 1-a;

            context.beginPath();
            context.moveTo(x0, y0);
            context.bezierCurveTo(a*xc+b*x0, a*yc+b*y0, a*xc+b*x1, a*yc+b*y1, x1, y1);
            context.stroke();
            
            m0 = bezier(0.7, {x:x0, y:y0}, {x:a*xc+b*x0, y:a*yc+b*y0}, {x:a*xc+b*x1, y:a*yc+b*y1}, {x:x1, y:y1});
            m1 = bezier(0.8, {x:x0, y:y0}, {x:a*xc+b*x0, y:a*yc+b*y0}, {x:a*xc+b*x1, y:a*yc+b*y1}, {x:x1, y:y1});
            interaction.drawArrowHead(context, m0.x, m0.y, m1.x, m1.y);
        }

        let canvas = document.querySelector("#maincanvas");
        let context = canvas.getContext("2d");
        context.clearRect(0, 0, canvas.width, canvas.height);
    
        context.strokeStyle="#EEEEEE";
        context.lineWidth = 50;
        context.beginPath();
        context.arc(interaction.main_center, interaction.main_center, interaction.main_radius, 0, 2*Math.PI);
        context.stroke();

        for(let c of interaction.currentView.connections)
        {
            try
            {
                context.strokeStyle="#999";
                context.fillStyle = "#999";
                context.lineWidth = 3;
                context.beginPath();
                p1 = interaction.module_pos[c.source.split('.')[0]];
                p2 = interaction.module_pos[c.target.split('.')[0]];
                draw_chord(context, p1.x, p1.y, p2.x, p2.y, interaction.main_center, interaction.main_center, interaction.main_radius);
                draw_bez_arrow(context, p1.x, p1.y, p2.x, p2.y, interaction.main_center, interaction.main_center, interaction.main_radius);
            }
            catch(err)
            {
                console.log("draw connection "+c.sourcemodule+"->"+c.targetmodule+" failed.");
            }
        }
    },
    deleteWidget()
    {
        let w = interaction.selectedObject;
        interaction.deselectObject();
        interaction.removeAllObjects();
        interaction.currentView.objects = interaction.currentView.objects.filter(e => e!==w.widget.parameters); // delete from view
        interaction.addView(interaction.currentViewName);
    },
    widgetToFront()
    {
        let w = interaction.selectedObject;
        interaction.deselectObject();
        interaction.removeAllObjects();
        interaction.currentView.objects = interaction.currentView.objects.filter(e => e!==w.widget.parameters); // delete from view
        interaction.currentView.objects.push(w.widget.parameters);
        interaction.addView(interaction.currentViewName);
    },
    widgetToBack()
    {
        let w = interaction.selectedObject;
        interaction.deselectObject();
        interaction.removeAllObjects();
        interaction.currentView.objects = interaction.currentView.objects.filter(e => e!==w.widget.parameters); // delete from view
        interaction.currentView.objects.unshift(w.widget.parameters);
        interaction.addView(interaction.currentViewName);
    },
    duplicateWidget()
    {
        let w = interaction.selectedObject;
        let dup_parameters = Object.assign({}, w.widget.parameters); // to avoid sharing
        dup_parameters.x += 20;
        dup_parameters.y += 20;
        let dup = interaction.addWidget(dup_parameters);
        interaction.currentView.objects.push(dup.widget.parameters);
        interaction.selectObject(dup);
    },
    addNewWidget()
    {
        let widget_select = document.querySelector('#widget_select');
        let widget_class = widget_select.options[widget_select.selectedIndex].value;
        let w = interaction.addWidget({'class': widget_class, 'show_title': true, 'show_frame': true, 'x': interaction.curnewpos, 'y': interaction.curnewpos, 'height': 300, 'width': 300});
        interaction.curnewpos += 20;
        interaction.currentView.objects.push(w.widget.parameters);
        interaction.selectObject(w);
    },
    addWidget(w)
    {
        let newObject = document.createElement("div");
        newObject.setAttribute("class", "frame visible");

        let newTitle = document.createElement("div");
        newTitle.setAttribute("class", "title");
        newTitle.innerHTML = "TITLE";
        newObject.appendChild(newTitle);

        interaction.main.appendChild(newObject);
        newObject.addEventListener('mousedown', interaction.startDrag, true);

        let constr = webui_widgets.constructors["webui-widget-"+w['class']];
        if(!constr)
        {
            console.log("Internal Error: No constructor found for "+"webui-widget-"+w['class']);
            newObject.widget = new webui_widgets.constructors['webui-widget-text'];
            newObject.widget.element = newObject; // FIXME: why not also below??
            newObject.widget.groupName = this.currentViewName.split('#')[0].split('/').slice(2).join('.');   // get group name - temporary ugly solution
            newObject.widget.parameters['text'] = "\""+"webui-widget-"+w['class']+"\" not found.";
        }
        else
        {
            newObject.widget = new webui_widgets.constructors["webui-widget-"+w['class']];
            newObject.widget.groupName = this.currentViewName.split('#')[0].split('/').slice(2).join('.');   // get group name - temporary ugly solution
            // Add default parameters from CSS - possibly...
            // FIXME: we should type convert here also according to default_template
            for(k in newObject.widget.parameters)
                if(w[k] === undefined)
                    w[k] = newObject.widget.parameters[k];
            newObject.widget.parameters = w;
        }

        newObject.widget.setAttribute('class', 'widget');
        newObject.appendChild(newObject.widget);    // must append before next section

        // Section below should not exists - probably...

        newObject.style.top = w.y+"px";
        newObject.style.left = w.x+"px";
        newObject.style.width = w.width+"px";
        newObject.style.height = w.height+"px";

        newObject.handle = document.createElement("div");
        newObject.handle.setAttribute("class", "handle");
        newObject.handle.onmousedown = interaction.startResize;
        newObject.appendChild(newObject.handle);
        
        try
        {
            newObject.widget.updateAll();
        }
        catch(err)
        {
            console.log(err);
        }
        
        return newObject;
    },
    
    addView(viewName)
    {
        setCookie('current_view', viewName);
        nav.openGroup(viewName);
        
        interaction.deselectObject();
        interaction.currentViewName = viewName;
        interaction.currentView = controller.views[viewName];
        interaction.removeAllObjects();

        let canvas = document.querySelector("#maincanvas");
        let context = canvas.getContext("2d");
        context.clearRect(0, 0, canvas.width, canvas.height);

        let main = document.querySelector('main');
        
        // Build widget view
        
        let v = interaction.currentView.objects;    // FIXME: objects should be called widgets
        if(v)
        {
            for(let i=0; i<v.length; i++)
                interaction.addWidget(v[i]);
            return;
        }

        // Build group view - experimental

        let m_width = 100;
        let m_height = 100;
        let m_corner = 50;
        let m_radius_x = m_width/2;
        let m_radius_y = m_height/2;
        
        interaction.main_radius = 400;
        interaction.main_margin = 100;
        interaction.main_center = interaction.main_radius+interaction.main_margin;
        interaction.module_radius_x = m_radius_x;
        interaction.module_radius_y = m_radius_x;

        interaction.module_pos = {}
        v = interaction.currentView.groups;
        if(v)
        {
            let scale = 2*Math.PI/v.length;
            for(let i=0; i<v.length; i++)
            {
                let newObject = document.createElement("div");
                if(v[i].is_group)
                    newObject.setAttribute("class", "module group");
                else
                    newObject.setAttribute("class", "module");

                newObject.innerHTML = v[i].attributes.name;
                interaction.main.appendChild(newObject);

                newObject.parameters = v[i];
                
                if(!newObject.parameters.attributes._x)
                {
                    newObject.parameters.attributes._x = interaction.main_center-interaction.main_radius*Math.cos(scale*i);
                    newObject.parameters.attributes._y = interaction.main_center+interaction.main_radius*Math.sin(scale*i);
                }
                
                interaction.module_pos[v[i].attributes.name] = {'x':newObject.parameters.attributes._x, 'y': newObject.parameters.attributes._y};

                newObject.style.top = (newObject.parameters.attributes._y-m_radius_x)+"px";
                newObject.style.left = (newObject.parameters.attributes._x-m_radius_y)+"px";
 
                if(newObject.parameters.attributes._text_color)
                    newObject.style.color = newObject.parameters.attributes._text_color;

                if(newObject.parameters.attributes._shape == 'rect')
                {
                
                }
                else
                {
                    newObject.style.borderRadius = m_corner+"px";
                }
                
                newObject.style.lineHeight = m_height+"px";
                newObject.style.width = m_width+"px";
                newObject.style.height = m_height+"px";

                if(newObject.parameters.attributes._color)
                    newObject.style.backgroundColor = newObject.parameters.attributes._color;
                
                newObject.addEventListener('mousedown', interaction.startDragModule, true);
            }
            interaction.drawConnections();
        }
    },
    deselectObject() {
        if(interaction.selectedObject)
        {
            interaction.selectedObject.className = interaction.selectedObject.className.replace(/selected/,'');
            interaction.selectedObject.className = interaction.selectedObject.className.replace(/dragged/,'');
            interaction.selectedObject.className = interaction.selectedObject.className.replace(/resized/,'');
            interaction.releaseElement();
            interaction.selectedObject = null;

            interaction.widget_inspector.style.display = "none";
            interaction.module_inspector.style.display = "none";
            interaction.edit_inspector.style.display = "block";
            interaction.network_inspector.style.display = "none";
        }
    },
    releaseElement: function(evt) {
        interaction.main.removeEventListener('mousemove',interaction.move,true);
        interaction.main.removeEventListener('mousemove',interaction.resize,true);
        interaction.main.removeEventListener('mouseup',interaction.releaseElement,true);
//        if(interaction.selectedObject)
        {
            interaction.selectedObject.className = interaction.selectedObject.className.replace(/dragged/,'');
            interaction.selectedObject.className = interaction.selectedObject.className.replace(/resized/,'');
        }
        if(evt) evt.stopPropagation()
    },
    selectObject: function(obj) {
        interaction.deselectObject()
        interaction.selectedObject = obj;
        interaction.selectedObject.className += ' selected';
        //document.querySelector('#selected').innerText = interaction.selectedObject.dataset.name;
        
        inspector.select(obj);
        
        interaction.widget_inspector.style.display = "block";
        interaction.edit_inspector.style.display = "none";
        interaction.module_inspector.style.display = "none";
        interaction.network_inspector.style.display = "none";
    },
    startDrag: function (evt) {
        // do nothing in run mode
        if(interaction.main.dataset.mode == "run")
            return;
        
        // continue propagation if in resize handle
        let r = this.handle.getBoundingClientRect();
        if( r.left < evt.clientX && evt.clientX < r.right &&
            r.top  < evt.clientY && evt.clientY < r.bottom)
            return;
        
        // handle the drag
        
        evt.stopPropagation();
        interaction.startX = this.offsetLeft;
        interaction.startY = this.offsetTop;
        interaction.selectObject(this);
        interaction.selectedObject.className += ' dragged';
        interaction.initialMouseX = evt.clientX;
        interaction.initialMouseY = evt.clientY;
        interaction.main.addEventListener('mousemove',interaction.move, true);
        interaction.main.addEventListener('mouseup',interaction.releaseElement,true);
        return false;
    },
    move: function (evt) {
        evt.stopPropagation()
        let dX = evt.clientX - interaction.initialMouseX;
        let dY = evt.clientY - interaction.initialMouseY;
        interaction.setPosition(dX,dY);
        return false;
    },
    startResize: function (evt) {
        evt.stopPropagation();
        interaction.startX = this.offsetLeft;
        interaction.startY = this.offsetTop;
        interaction.selectObject(this.parentElement);
        interaction.selectedObject.className += ' resized';
        interaction.initialMouseX = evt.clientX;
        interaction.initialMouseY = evt.clientY;
        interaction.main.addEventListener('mousemove',interaction.resize,true);
        interaction.main.addEventListener('mouseup',interaction.releaseElement,true);
        return false;
    },
    resize: function (evt) {
        let dX = evt.clientX - interaction.initialMouseX;
        let dY = evt.clientY - interaction.initialMouseY;
        interaction.setSize(dX,dY);
        return false;
    },
    setPosition: function (dx, dy) {
        let newLeft = interaction.grid_spacing*Math.round((interaction.startX + dx)/interaction.grid_spacing);
        let newTop = interaction.grid_spacing*Math.round((interaction.startY + dy)/interaction.grid_spacing);
        interaction.selectedObject.style.left = newLeft + 'px';
        interaction.selectedObject.style.top = newTop + 'px';
        // Update view data
        interaction.selectedObject.widget.parameters['x'] = newLeft;
        interaction.selectedObject.widget.parameters['y'] = newTop;
    },
    setSize: function (dx, dy) {
        let newWidth = interaction.sizegrid*Math.round((interaction.startX + dx)/interaction.sizegrid)+1;
        let newHeight = interaction.sizegrid*Math.round((interaction.startY + dy)/interaction.sizegrid)+1;
        interaction.selectedObject.style.width = newWidth + 'px';
        interaction.selectedObject.style.height = newHeight + 'px';
        
        // Update view data
        interaction.selectedObject.widget.parameters['width'] = newWidth;
        interaction.selectedObject.widget.parameters['height'] = newHeight;
        
        interaction.selectedObject.widget.updateAll();
    },
    setMode: function(mode) {
        interaction.deselectObject();
        let main = document.querySelector('main');
        main.dataset.mode = mode;
        
        if(main.dataset.mode == "edit")
        {
            interaction.widget_inspector.style.display = "none";
            interaction.module_inspector.style.display = "none";
            interaction.edit_inspector.style.display = "block";
            interaction.network_inspector.style.display = "none";
            interaction.main.addEventListener('mousemove', interaction.stopEvents, true);
            interaction.main.addEventListener('mouseout', interaction.stopEvents, true);
            interaction.main.addEventListener('mouseover', interaction.stopEvents, true);
            interaction.main.addEventListener('click', interaction.stopEvents, true);
        }
        else if(main.dataset.mode == "run")
        {
            interaction.widget_inspector.style.display = "none";
            interaction.edit_inspector.style.display = "none";
            interaction.module_inspector.style.display = "none";
            interaction.network_inspector.style.display = "none";
            interaction.main.removeEventListener('mousemove', interaction.stopEvents, true);
            interaction.main.removeEventListener('mouseout', interaction.stopEvents, true);
            interaction.main.removeEventListener('mouseover', interaction.stopEvents, true);
            interaction.main.removeEventListener('click', interaction.stopEvents, true);
        }
    },
    toggleEditMode: function() {
        interaction.edit_mode = ! interaction.edit_mode;
        if(interaction.edit_mode)
            interaction.setMode("edit")
        else
            interaction.setMode("run");
    },
    changeStylesheet: function() {
        let sheet = document.getElementById("stylesheet_select").value;
        document.getElementById("stylesheet").setAttribute("href", sheet);
    },
    
    // module interaction

    startDragModule: function (evt) {
        evt.stopPropagation();
        interaction.startX = this.offsetLeft;
        interaction.startY = this.offsetTop;
        interaction.selectModule(this);
        interaction.selectedObject.className += ' dragged';
        interaction.initialMouseX = evt.clientX;
        interaction.initialMouseY = evt.clientY;
        interaction.main.addEventListener('mousemove',interaction.moveModule, true);
        interaction.main.addEventListener('mouseup',interaction.releaseModule,true);
        return false;
    },
    moveModule: function (evt) {
        evt.stopPropagation()
        let dX = evt.clientX - interaction.initialMouseX;
        let dY = evt.clientY - interaction.initialMouseY;
        interaction.setModulePosition(dX,dY,!evt.altKey);
        interaction.drawConnections();
        return false;
    },
    setModulePosition: function (dx, dy, constrain) {
        let m_width = 100;  // should not be repeated here
        let m_height = 100;
        let m_corner = 50;
        let m_radius_x = m_width/2;
        let m_radius_y = m_height/2;
        
        let newLeft = interaction.startX + dx;
        let newTop = interaction.startY + dy;

        if(constrain)
        {
            let a = Math.atan2(newTop-interaction.main_center + interaction.module_radius_x, newLeft-interaction.main_center + interaction.module_radius_y)
            newLeft = interaction.main_center+interaction.main_radius*Math.cos(a) - interaction.module_radius_x;
            newTop = interaction.main_center+interaction.main_radius*Math.sin(a) - interaction.module_radius_y;
        }
        
        interaction.selectedObject.style.left = newLeft + 'px';
        interaction.selectedObject.style.top = newTop + 'px';
        // Update view data
        interaction.selectedObject.parameters.attributes['_x'] = newLeft + m_radius_x;
        interaction.selectedObject.parameters.attributes['_y'] = newTop + m_radius_y;
        interaction.module_pos[interaction.selectedObject.innerText] = {'x':newLeft +interaction.module_radius_x , 'y': newTop+interaction.module_radius_y};
    },
    selectModule: function(obj) {
        interaction.deselectObject()
        interaction.selectedObject = obj;
        interaction.selectedObject.className += ' selected';
        //document.querySelector('#selected').innerText = interaction.selectedObject.dataset.name;
        module_inspector.select(obj);
        interaction.module_inspector.style.display = "block";
        interaction.widget_inspector.style.display = "none";
        interaction.edit_inspector.style.display = "none";
        interaction.network_inspector.style.display = "none";
    },
    releaseModule: function(evt) {
        interaction.main.removeEventListener('mousemove',interaction.moveModule,true);
        interaction.main.removeEventListener('mouseup',interaction.releaseModule,true);
        interaction.selectedObject.className = interaction.selectedObject.className.replace(/dragged/,'');
        interaction.selectedObject.className = interaction.selectedObject.className.replace(/resized/,'');
        if(evt) evt.stopPropagation()
    }
}

/*
 *
 * Controller Scripts
 *
 */

controller = {
    run_mode: 'pause',
    command: 'update',
    tick: 0,
    session_id: 0,
    client_id: Date.now(),
    network: null,
    views: {},
    load_count: 0,
    load_count_timeout: null,
    g_data: null,
    send_stamp: 0,
    webui_interval: 0,
    webui_req_int: 100,
    timeout: 500,
    reconnect_interval: 1200,
    reconnect_timer: null,
    
    reconnect: function ()
    {
        controller.get("update", controller.update);
        let s = document.querySelector("#state");
        if(s.innerText == "waiting")
            document.querySelector("#state").innerHTML = "waiting &bull;";
        else
            document.querySelector("#state").innerHTML = "waiting";
    },
    
    defer_reconnect: function ()
    {
        clearInterval(controller.reconnect_timer);
        controller.reconnect_timer = setInterval(controller.reconnect, controller.reconnect_interval);
    },

    get: function (url, callback)
    {
        controller.send_stamp = Date.now();
        var last_request = url;
        xhr = new XMLHttpRequest();
        xhr.open("GET", url, true);

        xhr.onload = function(evt)
        {
            if(!xhr.response)   // empty response is ignored
            {
                console.log("onload - empty response - error")
                return;
            }
            controller.defer_reconnect(); // we are still on line
            setTimeout(controller.requestUpdate, controller.webui_req_int); // schedule next update; approximately 10/s
            callback(xhr.response, xhr.getResponseHeader("Session-Id"), xhr.getResponseHeader("Package-Type"));
        }
        
        xhr.responseType = 'json';
        xhr.timeout = 1000;
        try {
            xhr.send();
        }
        catch(error)
        {
            console.log(error);
        }
    },

    init: function () {
        controller.get("update", controller.update);
        //controller.defer_reconnect(); // start heartbeat
        controller.reconnect_timer = setInterval(controller.reconnect, controller.reconnect_interval);
    },
    
    stop: function () {
        controller.run_mode = 'stop';
        controller.get("stop", controller.update); // do not request data -  stop immediately
     },
    
    pause: function () {
        controller.command = 'pause';
    },
    
    step: function () {
        controller.command = 'step';
    },
    
    play: function () {
        controller.command = 'play';
    },
    
    realtime: function () {
        controller.command = 'realtime';
    },

    buildViewDictionary: function(group, name) {
        controller.views[name+"/"+group.attributes.name] = group;

        if(group.views)
            for(i in group.views)
                controller.views[name+"/"+group.attributes.name+"#"+group.views[i].name] = group.views[i];

        if(group.groups)
            for(i in group.groups)
                controller.buildViewDictionary(group.groups[i], name+"/"+group.attributes.name);
    },

    selectView: function(view) {
        interaction.addView(view);  // Create new view if it does not exist
    },

    updateWidgets(data)
    {
        // Update the views with data in response
        let w = document.getElementsByClassName('frame')
        for(let i=0; i<w.length; i++)
            try
            {
                w[i].children[1].receivedData = data;
                w[i].children[1].update(data); // include data for backward compatibility
            }
            catch(err)
            {
                console.log("updateWidgets failed: "+controller.client_id);
            }
    },

    clear_wait()
    {
        controller.load_count = 0;
        console.log("clear_wait - drawing or data load failed"); // FIXME: Remove
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
                if(w[i].children[1].loadData)
                {
                    controller.load_count += w[i].children[1].loadData(data);
                }
            }
     
            controller.load_count_timeout = setTimeout(controller.clear_wait, 200); // give up after 1/5 s and continue
            setTimeout(controller.wait_for_load, 1);
        }
        catch(err)
        {

        }
    },


    update(response, session_id, package_type)
    {
        controller.ping = Date.now() - controller.send_stamp;

        // Check if this is a new session
        if(!response)
        {
            controller.requestUpdate(); // empty respone - probably an error
        }
        else if(controller.session_id != session_id) // new session
        {
//            console.log("NEW SESSION "+session_id+" :"+['stop','pause','step','play','realtime'][response.state]);
            if(response.state)
                controller.run_mode = ['stop','pause','step','play','realtime'][response.state];
            else
                controller.run_mode = 'pause'
            controller.session_id = session_id;
            nav.init(response);
            controller.network = response;
            controller.views = {};
            controller.buildViewDictionary(response, "");
            
            let v = getCookie('current_view');
            if(Object.keys(controller.views).includes(v))
                controller.selectView(v);
            else
                controller.selectView(Object.keys(controller.views)[0]);
        }
        else if(package_type == "data") // same session - a new data package
        {
            // Set system info from package
            try
            {
                document.querySelector("#iteration").innerText = response.iteration;
                document.querySelector("#state").innerText = controller.run_mode; // +response.state+" "+" "+response.has_data;
                document.querySelector("#ticks_per_s").innerText = response.ticks_per_s;
                document.querySelector("#timebase").innerText = response.timebase+" ms";
                document.querySelector("#timebase_actual").innerText = response.timebase_actual+" ms";
                document.querySelector("#lag").innerText = response.lag+" ms";
                document.querySelector("#cpu_cores").innerText = response.cpu_cores;
                document.querySelector("#idle_time").value = response.idle_time;
                document.querySelector("#usage").value = response.cpu_usage;

                document.querySelector("#webui_updates_per_s").innerText = (1000/controller.webui_interval).toFixed(1) + (response.has_data ? "": " (no data)");
                document.querySelector("#webui_interval").innerText = controller.webui_interval+" ms";
                document.querySelector("#webui_req_int").innerText = controller.webui_req_int+" ms";
                document.querySelector("#webui_ping").innerText = controller.ping+" ms";
                document.querySelector("#webui_lag").innerText = (Date.now()-response.timestamp)+" ms";
                
                let p = document.querySelector("#progress");
                if(response.progress > 0)
                {
                    p.value = response.progress;
                    p.style.display = "table-row";
                }
                else
                {
                    p.style.display = "none";
                }
                
                controller.run_mode = ['stop','pause','step','play','realtime'][response.state];
            }
            catch(err)
            {
                console.log("incorrect package received form ikaros (1)")
            }
            
            if(response.has_data)
                controller.updateImages(response.data);
        }
        else
        {
            console.log("incorrect package received form ikaros (2)")
        }
    },

    requestUpdate: function()
    {
        controller.webui_interval = Date.now() - controller.last_request_time;
        controller.last_request_time = Date.now();

        if(!interaction.currentView) // no view selected
        {
            controller.get("update", controller.update);
            return;
        }

        // Request new data
        let data_set = new Set();
        
        // Very fragile loop - maybe use class to mark widgets or something
        let w = document.getElementsByClassName('frame');
        for(let i=0; i<w.length; i++)
            try
            {
                w[i].children[1].requestData(data_set);
            }
            catch(err)
            {}

        let group_path = interaction.currentViewName.split('#')[0].split('/').slice(2).join('.'); // OLD currentViewName.substring(1).split("/").slice(1).join("."); // FIXME: use this format all the time
        let data_string = group_path+"#"; // should be added to names to support multiple clients
        let sep = "";
        for(s of data_set)
        {
            data_string += (sep + s);
            sep = "#"
         }

        controller.get(controller.command+"?id="+controller.client_id+"&data="+encodeURIComponent(data_string), controller.update);
        controller.command = 'update';
    },

    copyView: function() // TODO: Remove default parameters
    {
        let s = "<view name=\""+interaction.currentViewName.split('#')[1]+"\">\n\n"; // FIXME: probably don't work for included views
        let w = document.getElementsByClassName('frame');
        for(let i=0; i<w.length; i++)
            try
            {
                let wgt = w[i].children[1]
                let name = wgt.localName.replace("webui-widget-", "");
                s += "\t<"+name+"\n"
                for(let p in wgt.parameters)
                    if(wgt.parameters[p])
                        s += "\t\t"+p+" = \""+wgt.parameters[p]+"\"\n";
                s += "\t/>\n\n"
            }
            catch(err)
            {
                s += " AN ERROR OCCURED";
            }
        s += "</view>"
        copyToClipboard(s);
    },

    connectionToXML: function(connection, indent="")
    {
        let text = indent+"<connection "
        for(let p in connection)
            text += p+' = "'+connection[p]+'" ';
        text += "/>\n";
        return text;
    },

    viewToXML: function(view, indent="")
    {
        let text = indent + '<view name="'+view.name+'"';
        for(let a in view)
            if(!['name', 'objects'].includes(a))
                text += ' ' + a + ' = "'+view[a]+'"';
        text += ' >\n';
        
        for(let w in view.objects)
        {
            text += '\t'+indent + '<'+view.objects[w].class+' ';

            for(let a in view.objects[w])
                if(!['class'].includes(a))
                    text += ' '+ a + ' = "'+view.objects[w][a]+'"';

            text += '/>\n';
        }
        text += indent + '</view>\n';
        return text;
    },

    moduleToXML: function(module, indent="")
    {
        let text = indent + '<module\n';
        for(let a in module.attributes)
            text += indent + '\t' + a + ' = "'+module.attributes[a]+'"\n';
        text += indent+'>\n';

         for(let v in module.views)
            text += controller.viewToXML(module.views[v], indent+"\t");

        text += indent + '</module>\n';
        return text;
    },

    groupToXML: function(group, indent="")
    {
        let text = indent + '<group\n';
        for(let a in group.attributes)
            text += indent + '\t' + a + ' = "'+group.attributes[a]+'"\n';
        text += indent+'>\n';

        for(let g in group.groups)
            if(group.groups[g].is_group)
                text += controller.groupToXML(group.groups[g], indent+"\t");
            else
                text += controller.moduleToXML(group.groups[g], indent+"\t");

        for(let c in group.connections)
            text += controller.connectionToXML(group.connections[c], indent+"\t");
         for(let v in group.views)
            text += controller.viewToXML(group.views[v], indent+"\t");
        text += indent + '</group>\n';
        return text;
    },

    saveNetwork: function()
    {
        function download(filename, text) {
            var element = document.createElement('a');
            element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
            element.setAttribute('download', filename);
            element.style.display = 'none';
            document.body.appendChild(element);
            element.click();
            document.body.removeChild(element);
        }

        download("network.ikg", '<?xml version="1.0" encoding="UTF-8"?>\n'+controller.groupToXML(controller.network));
    }
}

