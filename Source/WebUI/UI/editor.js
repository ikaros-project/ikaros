var svgns = "http://www.w3.org/2000/svg";
var xlinkns = "http://www.w3.org/1999/xlink";

var module_load_count = 0;

var gx = 210.5;
var gy = 210.5;

var dynamics = false;

// Add getChildrenByTagName to SVGElement

window.Element.prototype.getChildrenByTagName = function(name)
{
    var list = new Array();
    for(var i=0; i<this.childNodes.length; i++)
        if (this.childNodes[i].nodeName==name)
            list.push(this.childNodes[i]);
    return list;
}



function getXML(url, callback)
{
	function getURL(url, callback)
	{
		function callCallback()
		{
			if (ajaxRequest.readyState == 4)
			{
				if(ajaxRequest.status == 200)
					if (ajaxCallback) ajaxCallback(ajaxRequest.responseXML);
				else
					alert("Returned status code " + ajaxRequest.status);
			}
		}

		var ajaxRequest = null;
		var ajaxCallback = callback;

		ajaxRequest = new XMLHttpRequest();
		ajaxRequest.onreadystatechange = callCallback;
		ajaxRequest.open("GET", url, true);
		ajaxRequest.send(null);
	}			

	if(window.XMLHttpRequest)
		getURL(url, callback);
	else
		alert("Error: This web browser does not have XMLHttpRequest");
}



Background.prototype.handleEvent = function(e)
{
	if (e.type in this) this[e.type](e);
}

Background.prototype.mouseover = function(e)
{
}

Background.prototype.mouseout = function(e)
{
}

Background.prototype.mouseup = function(e)
{
	if(!this.drag_object) return;
	
	this.drag_object.unselect();
	this.drag_object = null;
}

hypot = function(x, y)
{
    return Math.sqrt(x*x+y*y);
}

Background.prototype.arrange = function()
{
    var total_energy = 0;
    var offset = 100;
    var repulsion = 0.12;
    var attraction = 0.001;
    
    // terrible way to get the size of the editor
    var h = parent.document.documentElement.clientHeight;
    h -= parent.document.getElementById('view').offsetTop;
    h -= 200;
    h /= 2;
    
    var w = parent.document.documentElement.clientWidth;
    w -= parent.document.getElementById('view').offsetLeft;
    w -= 200;
    w /= 2;

    // Force toward center

    var cforce = {x: 0, y: 0};
    for(i in this.modules)
    {
        var mip = this.modules[i].get_position();
        cforce.x += (mip.x-w);
        cforce.y += (mip.y-h);
    }
    cforce.x /= this.modules.length;
    cforce.y /= this.modules.length;

//    console.log(cforce.x, cforce.y, this.modules.length, h, w);
    
    for(i in this.modules)
    {
        var force = {x: 0, y: 0};

        // Repulsion

        for(j in this.modules)
            if(this.modules[j] != this.modules[i])
            {   
                var mip = this.modules[i].get_position();
                var mjp = this.modules[j].get_position();
                var len = hypot(mip.x-mjp.x, mip.y-mjp.y);
                var d = repulsion/Math.sqrt(len);
                force.x -= d*(mip.x-mjp.x)/len - 0.001*cforce.x;
                force.y -= 4*d*(mip.y-mjp.y)/len - 0.001*cforce.y;
            }
            
        // Attraction (and i/o-directions)    

        for(var j in this.modules[i].inputs)
            for(k in this.modules[i].inputs[j].connections)
            {
                var cs = this.modules[i].inputs[j].connections[k].start;
                var ce = this.modules[i].inputs[j].connections[k].end;
                force.x += 0.001*(ce.x-cs.x);
                force.y += 0.001*(ce.y-cs.y);
                if(ce.x < cs.x+offset)
                    force.x += 0.001*(ce.x-cs.x-offset);
            }

       for(var j in this.modules[i].outputs)
            for(k in this.modules[i].outputs[j].connections)
            {
                var cs = this.modules[i].outputs[j].connections[k].start;
                var ce = this.modules[i].outputs[j].connections[k].end;
                force.x -= 0.001*(ce.x-cs.x);
                force.y -= 0.001*(ce.y-cs.y);
                if(ce.x < cs.x+offset)
                    force.x -= 0.001*(ce.x-cs.x-offset);
            }

        var p = this.modules[i].get_position();
        p.x -= 50*force.x;
        p.y -= 50*force.y;
        total_energy += force.x*force.x+force.y*force.y;
        
        this.modules[i].set_position(p.x, p.y);
    }
    
    var that = this;
    if(dynamics)
        setTimeout(function(){that.arrange()}, 10);
}



Background.prototype.click = function(evt)
{
    if(!dynamics)
    {
        dynamics = true;
        this.arrange();
    }
    else
        dynamics = false;
    
//    window.console.log("dynamics = "+dynamics);	
/*	
    for(i in this.modules)
	{
		window.console.log("Module "+i+": "+this.modules[i].m.getAttribute("name"));
		for(j in this.modules[i].inputs)
			window.console.log("      Input "+j+": "+this.modules[i].inputs[j].name);	
		for(j in this.modules[i].outputs)
			window.console.log("      Output "+j+": "+this.modules[i].outputs[j].name);	
	}
*/

	return;

	var w = window.open("inspector.html", "inspector", "width=200,height=400");
	var doc = w.document;
	
	doc.write("<html><head><title>Inspector</title>");
	doc.write("<link rel='stylesheet' type='text/css' href='style.css' />");
	doc.write("</head><body>");
	doc.write("<form>");
	doc.write("<span>class:</span><input type='text' name='class' value='classname'><br />");
	doc.write("<span>name:</span><input type='text' name='name'><br />");
	doc.write("</form>");
	doc.write("</body></html>");
	

	doc.close();
}

Background.prototype.mousemove = function(e)
{
	if(!this.drag_object) return;
	
	var new_x = e.clientX+this.drag_offset_x;
	var new_y = e.clientY+this.drag_offset_y;
	
	// Snap to grid (if not in dynamic mode)
	
    if(!dynamics)
    {
        var grid = 25;
        new_x = grid*Math.round(new_x/grid)+0.5;
        new_y = grid*Math.round(new_y/grid)+0.5;
	}
    
	this.drag_object.set_position(new_x, new_y);
}

Background.prototype.start_drag = function(obj, evt)
{
	obj.select();
	this.drag_object = obj;
	this.drag_offset_x = obj.get_position().x - evt.clientX;
	this.drag_offset_y = obj.get_position().y - evt.clientY;
}

Background.prototype.add_module = function(m)
{
	this.modules.push(m);
}

Background.prototype.get_module = function(name)
{
	for(var i in this.modules)
	{
		if(this.modules[i].m.getAttribute('name') == name)
			return this.modules[i];
	}
	return null;
}

Background.prototype.get_input_position = function(sm, s)
{
	var m =this.get_module(sm);
	if(!m) return null;
	return m.get_input_position(s);
}


Background.prototype.get_output_position = function(sm, s)
{
	var m = this.get_module(sm);
	if(!m) return null;
	
//		alert("found ("+sm+"."+s+") ="+m.get_output_position(s));
	
	return m.get_output_position(s);
}	


Background.prototype.add_connection = function(c)
{
	this.connections.push(c);
}

Background.prototype.read_connections = function()
{
    for(var i=this.xml.documentElement.firstChild; i.nextSibling; i=i.nextSibling)
        if(i.nodeName == "connection")
        {
            this.add_connection(new Connection(bg, i));
        }
//	var cons = this.xml.getElementsByTagName("connection");
//	for(var i=0; i<cons.length; i++)
//	{
//		this.add_connection(new Connection(bg, cons[i]));
//	}
}



Background.prototype.read_modules = function(xml)
{
	if(xml == null)
		return;
		
	this.xml = xml
    
    // Get group/module path
    
    var p = location.href.split('/');
    p.pop();
    p.shift();
    p.shift();
    p.shift();

    this.rootpath = p;
    this.root = xml.documentElement.firstChild;

    // go down the xml tree
    
    for(n in this.rootpath)
    {
        console.log(this.rootpath[n]);
        for(var i=this.root; i.nextSibling; i=i.nextSibling)
            if(i.nodeName == "group" && i.getAttribute("name")==this.rootpath[n])
            {
                this.root = i.firstChild;
                console.log("New root: "+this.rootpath[n]);
                break;
            }
    }

    // count module and create graphical objects

    for(var i=this.root; i.nextSibling; i=i.nextSibling)
        if(i.nodeName == "group")
        {
            console.log("counting: "+i.getAttribute("name"));
            module_load_count++;
        }
    console.log("Module count: "+ module_load_count);
    
    for(var i=this.root; i.nextSibling; i=i.nextSibling)
        if(i.nodeName == "group")
            this.add_module(new Module(this, i));
}



function Background()
{
	this.r = document.createElementNS(svgns,"rect");	
	this.r.setAttribute('x', 0);
	this.r.setAttribute('y', 0);
    
	this.r.setAttribute('height', 2000);
	this.r.setAttribute('width', 2000);
	this.r.setAttribute('fill', '#FFF');
	document.documentElement.appendChild(this.r);
    

	// Add Grid (put in group later)
	
	for(var i=0; i<2000; i+=25)
	{
		var l = document.createElementNS(svgns,"line");	
		l.setAttribute('x1', i+0.5);
		l.setAttribute('y1', 0);
		l.setAttribute('x2', i+0.5);
		l.setAttribute('y2', 2000);
		l.setAttribute('stroke', '#AAF');
		document.documentElement.appendChild(l);
	}

	for(var i=0; i<2000; i+=25)
	{
		var l = document.createElementNS(svgns,"line");	
		l.setAttribute('x1', 0);
		l.setAttribute('y1', i+0.5);
		l.setAttribute('x2', 2000);
		l.setAttribute('y2', i+0.5);
		l.setAttribute('stroke', '#AAF');
		document.documentElement.appendChild(l);
	}
	
	this.r.addEventListener('mouseover', this, false);
	this.r.addEventListener('mouseout', this, false);
	this.r.addEventListener('mousemove', this, false);
	this.r.addEventListener('mouseup', this, false);
	this.r.addEventListener('click', this, false);

	this.drag_object = null;
	this.drag_offset_x = 0;
	this.drag_offset_y = 0;
	
	this.modules = new Array(0);
	this.connections = new Array(0);
}



function find_named_node(node, name)
{
	if(!node) return null;

	if(node.nodeType == 1 && node.getAttribute('name') == name)
		return node;
	
	if(node.firstChild != null)
	{
		var n = find_named_node(node.firstChild, name);
		if(n != null) return n;
	}

	if(node.nextSibling != null)
		return find_named_node(node.nextSibling, name);	
}


Module.prototype.get_position = function()
{
	return this.position;
}

Module.prototype.set_position = function(new_x, new_y)
{
	this.position = {x: new_x, y: new_y};
	this.r.setAttribute("transform", "translate("+new_x+","+new_y+")");
	
	// Update connections

	for(var i in this.inputs)
	{
		for(j in this.inputs[i].connections)
			this.inputs[i].connections[j].set_end({x: this.position.x+this.inputs[i].x, y: this.position.y+this.inputs[i].y});
	}

	for(var i in this.outputs)
	{
		for(j in this.outputs[i].connections)
            this.outputs[i].connections[j].set_start({x: this.position.x+this.outputs[i].x, y: this.position.y+this.outputs[i].y});
    }
}

Module.prototype.select = function()
{
	if(this.selection) this.selection.setAttribute('visibility', 'visible');
	this.r.setAttribute("style", "pointer-events: none");
}

Module.prototype.unselect = function()
{
	if(this.selection) this.selection.setAttribute('visibility', 'hidden');
	this.r.setAttribute("style", "pointer-events: all");
}

Module.prototype.handleEvent = function(evt)
{
	if (evt.type in this) this[evt.type](evt);
}

Module.prototype.mousedown = function(evt)
{
	this.background.start_drag(this, evt);
}


Module.prototype.set_input_connection = function(c, name)
{
	for(var i in this.inputs)
		if(this.inputs[i].name == name)
			this.inputs[i].connections.push(c);
}

Module.prototype.set_output_connection = function(c, name)
{
	for(var i in this.outputs)
		if(this.outputs[i].name == name)
			this.outputs[i].connections.push(c);
}

Module.prototype.get_input_position = function(name)
{
	for(var i in this.inputs)
		if(this.inputs[i].name == name)
			return { x: this.position.x+this.inputs[i].x, y: this.position.y+this.inputs[i].y };
	return null;
}


Module.prototype.get_output_position = function(name)
{
	for(var i in this.outputs)
		if(this.outputs[i].name == name)
			return { x: this.position.x+this.outputs[i].x, y: this.position.y+this.outputs[i].y };
	return null;
}


Module.prototype.load_class_data = function(xml) // not used
{
	this.class_xml = xml;
	var _this = this; getXML("/ClassSVG/"+this.m.getAttribute("class")+".svg",  function(x) { _this.load_module_svg(x); });
}


Module.prototype.load_module_svg = function(xml)
{
	this.r = document.importNode(xml.getElementsByTagName('g')[0], true);
	this.selection = find_named_node(this.r, 'selection');
	this.name = find_named_node(this.r, 'name');
	if(this.name) this.name.firstChild.nodeValue = this.m.getAttribute("name");
	
	if(this.m.getAttribute('x') && this.m.getAttribute('y'))
	{
		this.set_position(+this.m.getAttribute('x')+0.5, +this.m.getAttribute('y')+0.5);
	}
	else
	{
		this.set_position(gx, gy);
		gx += 20;
		gy += 20;
	}
	
//	window.console.log("Loading Module: "+this.m.getAttribute("name"));

	// Find I/O

	var input = this.m.getChildrenByTagName('input');
	for(var i=0; i<input.length; i++)
	{
		var n = input[i].getAttribute("name");
		var node = find_named_node(this.r, n);
		this.inputs.push({name: n, x: +node.getAttribute('cx'), y: +node.getAttribute('cy'), connections: [] });
//		window.console.log("  Pushing input: "+n);	
	}
	
	var output = this.m.getChildrenByTagName('output');

	for(var i=0; i<output.length; i++)
	{
		var n = output[i].getAttribute("name");
//      console.log("Finding node: "+this.r+" : "+n);
		var node = find_named_node(this.r, n);
		this.outputs.push({name: n, x: +node.getAttribute('cx'), y: +node.getAttribute('cy'), connections: [] });
//		window.console.log("  Pushing output: "+n);	
	}

	this.r.addEventListener('mousedown', this, false);
	
	document.documentElement.appendChild(this.r);
	
	if(--module_load_count == 0) // last loaded module trigger connections
		this.background.read_connections();
}



function Module(bg, m)
{
	this.inputs = [];
	this.outputs = [];
	
	this.background = bg;
	this.selection = null;
	this.name = null;
	this.m = m;
	this.position = {x:0, y:0};

//	var _this = this; getXML(bg.rootpath.join('/')+'/'+this.m.getAttribute("name")+"/modulegraph.svg",  function(x) { _this.load_module_svg(x); });

    rp = "";
    if(bg.rootpath.length > 0)
        rp = "/"+bg.rootpath.join('/');
    
	var _this = this; getXML(rp+'/'+this.m.getAttribute("name")+"/modulegraph.svg",  function(x) { _this.load_module_svg(x); });
}



Connection.prototype.set_start = function(m)
{
//	if(!m) alert(null);
	if(!m) return;
	this.start = {x: m.x, y: m.y};
	this.line.setAttribute('x1', m.x);
	this.line.setAttribute('y1', m.y);
}



Connection.prototype.set_end = function(m)
{
//	if(!m) alert(null);
	if(!m) return;
	this.end = {x: m.x, y: m.y};
	this.line.setAttribute('x2', m.x);
	this.line.setAttribute('y2', m.y);
}



function Connection(bg, c)
{		
	this.background = bg;
	this.c = c;

	this.sourcemodule = c.getAttribute("sourcemodule");
	this.source = c.getAttribute("source");
	this.targetmodule = c.getAttribute("targetmodule");
	this.target = c.getAttribute("target");
	
//	window.console.log("CON: "+this.sourcemodule+'.'+this.source+" -> "+this.targetmodule+'.'+this.target);

	// get coordinates

	this.line = document.createElementNS(svgns,"line");

	this.set_start(bg.get_output_position(this.sourcemodule, this.source));	// ERROR HANDLING ***
	this.set_end(bg.get_input_position(this.targetmodule, this.target));

	var sm = bg.get_module(this.sourcemodule);
	if(sm) sm.set_output_connection(this, this.source);
	
	var tm = bg.get_module(this.targetmodule);
	if(tm) tm.set_input_connection(this, this.target);
	
	this.line.setAttribute('stroke', 'black');

	document.documentElement.appendChild(this.line);

//	window.console.log("CON: "+this.sourcemodule+'.'+this.source+ "["+this.start.x+","+this.start.y+"]"+" -> "+this.targetmodule+'.'+this.target+ "["+this.end.x+","+this.end.y+"]");
}


/*
function handle_xml(xml)
{
    alert("in handle_xml");
	bg.xml = xml;
	var modules = xml.getElementsByTagName("module");
	module_load_count = modules.length;
	for(var i=0; i<modules.length; i++)
		bg.add_module(new Module(bg, modules[i]));
}
*/


var bg = new Background();

//getXML("test.ikc", function(xml) {bg.read_modules(xml);}); // Sent separately
