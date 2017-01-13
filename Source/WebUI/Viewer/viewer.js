var svgns = "http://www.w3.org/2000/svg";
var xlinkns = "http://www.w3.org/1999/xlink";
var xmlns = "http://www.w3.org/1999/xhtml";

var clip_id = 0;

LUT_gray = [
            '#000000', '#040404', '#080808', '#0C0C0C', '#101010', '#141414', '#181818', '#1C1C1C',
            '#202020', '#242424', '#282828', '#2C2C2C', '#303030', '#343434', '#383838', '#3C3C3C',
            '#404040', '#444444', '#484848', '#4C4C4C', '#505050', '#545454', '#585858', '#5C5C5C',
            '#606060', '#646464', '#686868', '#6C6C6C', '#707070', '#747474', '#787878', '#7C7C7C',
            '#808080', '#848484', '#888888', '#8C8C8C', '#909090', '#949494', '#989898', '#9C9C9C',
            '#A0A0A0', '#A4A4A4', '#A8A8A8', '#ACACAC', '#B0B0B0', '#B4B4B4', '#B8B8B8', '#BCBCBC',
            '#C0C0C0', '#C4C4C4', '#C8C8C8', '#CCCCCC', '#D0D0D0', '#D4D4D4', '#D8D8D8', '#DCDCDC',
            '#E0E0E0', '#E4E4E4', '#E8E8E8', '#ECECEC', '#F0F0F0', '#F4F4F4', '#F8F8F8', '#FCFCFC'];

LUT_fire = [
            '#000000', '#00001F', '#00002E', '#00003D', '#00004E', '#010060', '#0D0071', '#190082',
            '#250093', '#3100A5', '#3D00B2', '#4900C0', '#5500CE', '#6200DC', '#6E00DF', '#7A00E3',
            '#8600DA', '#9200D2', '#9A00C3', '#A200B5', '#A700A6', '#AD0097', '#B20088', '#B8007A',
            '#BD006B', '#C3005D', '#C9074E', '#CF0E40', '#D41831', '#D92323', '#DF2E14', '#E53905',
            '#EA4402', '#F04F00', '#F65A00', '#FC6500', '#FD6D00', '#FF7500', '#FF7D00', '#FF8500',
            '#FF8C00', '#FF9300', '#FF9A00', '#FFA100', '#FFA800', '#FFAF00', '#FFB600', '#FFBE00',
            '#FFC500', '#FFCD00', '#FFD400', '#FFDB00', '#FFE200', '#FFEA00', '#FFF111', '#FFF823',
            '#FFFB42', '#FFFF62', '#FFFF81', '#FFFFA0', '#FFFFBF', '#FFFFDF', '#FFFFEF', '#FFFFFF'];

LUT_spectrum = [
                '#FF0000', '#FF0000', '#FF1800', '#FF3000', '#FF4800', '#FF6000', '#FF7800', '#FF9000',
                '#FFA800', '#FFC000', '#FFD800', '#FFF000', '#F6FF00', '#DEFF00', '#C6FF00', '#AEFF00',
                '#96FF00', '#7EFF00', '#66FF00', '#4EFF00', '#36FF00', '#1EFF00', '#06FF00', '#00FF12',
                '#00FF2A', '#00FF42', '#00FF5A', '#00FF72', '#00FF8A', '#00FFA2', '#00FFBA', '#00FFD2',
                '#00FFEA', '#00FCFF', '#00E4FF', '#00CCFF', '#00B4FF', '#009CFF', '#0084FF', '#006CFF',
                '#0054FF', '#003CFF', '#0024FF', '#000CFF', '#0C00FF', '#2400FF', '#3C00FF', '#5400FF',
                '#6C00FF', '#8400FF', '#9C00FF', '#B400FF', '#CC00FF', '#E400FF', '#FC00FF', '#FF00EA',
                '#FF00D2', '#FF00BA', '#FF00A2', '#FF008A', '#FF0072', '#FF005A', '#FF0042', '#FF002A'];

LUT_contrast = [
                '#FF0000', '#FF8429', '#FFFF10', '#299C39', '#00A5C6', '#083194', '#8C007B', '#CE007B', '#808080', '#FFFFFF'
        		];

function makeSelectionArray(s)
{
    if(s == undefined)
        return [[0,0]];
    
    if(String.prototype.isPrototypeOf(s))
        s = s.split(",");
    
    if(Array.prototype.isPrototypeOf(s))
    {
        if(Array.prototype.isPrototypeOf(s[0]))
            return s;
        
        var a = new Array(0);
        for(var i in s)
            a.push([s[i],0]);
        return a;
    }
    
    return makeSelectionArray(eval('['+s+']'));
}        



function makeLUTArray(c, d)
{
    if(c == undefined)
    {
        if(d == undefined)
            return ['yellow'];
        else
            return d;
    }
    
    c = String(c);

    if(c.indexOf("LUT_") == 0)
        return eval(c);
    
    var f = false;
    var s = c.split("");
    for(var i=0; i<s.length; i++)
        if(s[i] == '(')
            f = true;
        else if(s[i] == ')')
            f = false;
        else if(f && s[i] == ',')
            s[i] = ';'
    
    c = s.join("");
    
    var a = c.split(",");
    for(i in a)
    {
        a[i] = a[i].replace(/ /g,"");
        a[i] = a[i].replace(/;/g,",");
    }
    
    return a;
}



String.prototype.unescapeEntities = function ()
{
    var temp = document.createElement("div");
    temp.innerHTML = this;
    var result = temp.childNodes[0].nodeValue;
    temp.removeChild(temp.firstChild);
    return result;
}



function WebUIObject(obj, p, title)
{
    if(!p.title)
		p.title = title;
    
    p.title = p.title.unescapeEntities();
	
    if(!p.behind)
        p.behind = false;
    
    obj.module = p.module;
	obj.source = p.source;
	obj.width = p.width;
	obj.height = p.height;
    
    var view = document.getElementById("frame");
    
    var r = document.createElement("div");
    r.className = "object_background"
    r.style.left = p.x;
    r.style.top = p.y;
    r.style.width = p.width;
    r.style.height = p.height;
    view.appendChild(r);
    
    this.bg = r;
    
	if(!(p.opaque != undefined ? p.opaque=='yes' : p.behind))
    {
        r.style.background="none";
        return;
    }
    
    var h = document.createElement("div");
    h.className = "object_titlebar"
    h.style.width = p.width;
    r.appendChild(h);
    
    var t = document.createElement("div");
    t.className = "object_title"
    t.style.width = p.width;
    r.appendChild(t);
    
    var tt = document.createTextNode(p.title);
    t.appendChild(tt);
}



function WebUICanvas(obj, p, ctype)
{
	if(!p.title)
		p.title = p.module+'.'+p.source;

    p.title = p.title.unescapeEntities();

	if(!p.behind)
        p.behind = false;
 
    if(!ctype)
        ctype = "2d";
    
    // Deadult Parameters
    
    obj.module = p.module;
	obj.source = p.source;
	obj.width = p.width;
	obj.height = p.height;
    
    obj.min = (p.min ? p.min : 0);
	obj.max = (p.max ? p.max : 1);
	obj.scale = 1/(obj.max == obj.min ? 1 : obj.max-obj.min);
    
	obj.min_x = (p.min_x ? p.min_x : obj.min);
	obj.max_x = (p.max_x ? p.max_x : obj.max);
	obj.scale_x = 1/(obj.max_x == obj.min_x ? 1 : obj.max_x-obj.min_x);
    
	obj.min_y = (p.min_y ? p.min_y : obj.min);
	obj.max_y = (p.max_y ? p.max_y : obj.max);
	obj.scale_y = 1/(obj.max_y == obj.min_y ? 1 : obj.max_y-obj.min_y);
    
    obj.flip_x_axis = (p.flip_x_axis ? p.flip_x_axis == "yes" : false);
    obj.flip_y_axis = (p.flip_y_axis ? p.flip_y_axis == "yes" : false);

    obj.stroke_LUT = makeLUTArray(p.color, ['yellow']);
    obj.line_dash_LUT = makeSelectionArray(p.line_dash, []);
    
    obj.fill_LUT = makeLUTArray(p.fill, ['none']);
	obj.line_width_LUT = makeLUTArray(p.line_width, [1]);
	obj.arrow_head_LUT = makeLUTArray(p.arrow, ['no']);
    
    obj.line_cap = (p.line_cap ? p.line_cap : "butt");
    obj.line_join = (p.line_join ? p.line_join : "miter");

    var view = document.getElementById("frame");
    
    var r = document.createElement("div");
    r.className = "object_background"
    r.style.left = p.x;
    r.style.top = p.y;
    r.style.width = p.width;
    r.style.height = p.height;
    view.appendChild(r);
    
    this.bg = r;
    
    if(!obj.oversampling)
        obj.oversampling = 1;

    obj.canvas = document.createElement("canvas");
    obj.canvas.style.width = p.width;
    obj.canvas.style.height = p.height;
    obj.canvas.width = obj.oversampling*p.width;
    obj.canvas.height = obj.oversampling*p.height;
    obj.canvas.style.borderRadius = "11px";
    
    this.bg.appendChild(obj.canvas);
    
    obj.context = obj.canvas.getContext(ctype);
    
    if(ctype == "2d")
    {
        if(obj.flip_x_axis)
        {
            obj.context.translate(obj.canvas.width, 0);
            obj.context.scale(-1, 1);
        }
        
        if(obj.flip_y_axis)
        {
            obj.context.translate(0, obj.canvas.height);
            obj.context.scale(1, -1);
        }

        obj.context.clearRect(0, 0, obj.width, obj.height);
        obj.context.fillRect(0, 0, obj.width, obj.height);
        obj.context.lineCap = obj.line_cap;
        obj.context.lineJoin = obj.line_join;

        if(!(p.opaque != undefined ? p.opaque=='yes' : p.behind))
        {
            r.style.background="none";
            return;
        }

        var h = document.createElement("div");
        h.className = "object_titlebar"
        h.style.width = p.width;
        r.appendChild(h);
        
        var t = document.createElement("div");
        t.className = "object_title"
        t.style.width = p.width;
        r.appendChild(t);
        
        var tt = document.createTextNode(p.title);
        t.appendChild(tt);
        
        obj.context.drawArrow = function(arrow)
        {
            this.beginPath();
            this.moveTo(arrow[arrow.length-1][0],arrow[arrow.length-1][1]);
            for(var i=0;i<arrow.length;i++){
                this.lineTo(arrow[i][0],arrow[i][1]);
            }
            this.closePath();
            this.fill();
            this.stroke();
        };
        
        obj.context.moveArrow = function(arrow, x, y)
        {
            var rv = [];
            for(var i=0;i<arrow.length;i++){
                rv.push([arrow[i][0]+x, arrow[i][1]+y]);
            }
            return rv;
        };
        
        obj.context.rotateArrow = function(arrow,angle)
        {
            var rv = [];
            for(var i=0; i<arrow.length;i++){
                rv.push([(arrow[i][0] * Math.cos(angle)) - (arrow[i][1] * Math.sin(angle)),
                         (arrow[i][0] * Math.sin(angle)) + (arrow[i][1] * Math.cos(angle))]);
            }
            return rv;
        };
        
        obj.context.drawArrowHead = function(fromX, fromY, toX, toY)
        {
            var angle = Math.atan2(toY-fromY, toX-fromX);
            var arrow = [[0,0], [-10,-5], [-10, 5]];
            this.save();
            this.lineJoin = "miter";
            this.fillStyle = this.strokeStyle;
            this.drawArrow(this.moveArrow(this.rotateArrow(arrow,angle),toX,toY));
            this.restore();
        };
        
        obj.context.drawLineArrow = function(fromX, fromY, toX, toY)
        {
            this.beginPath();
            this.moveTo(fromX,fromY);
            this.lineTo(toX,toY);
            this.stroke();
            var angle = Math.atan2(toY-fromY, toX-fromX);
            var arrow = [[0,0], [-10,-5], [-10, 5]];
            this.drawArrow(this.moveArrow(this.rotateArrow(arrow,angle),toX,toY));
        };

        obj.context.drawLine = function(fromX, fromY, toX, toY)
        {
            this.beginPath();
            this.moveTo(fromX,fromY);
            this.lineTo(toX,toY);
            this.stroke();
        };
    }
}



function OLDWebUICanvas(obj, p)
{
	if(!p.title)
		p.title = p.module+'.'+p.source;

	if(!p.behind)
        p.behind = false;
    
    // Deadult Parameters
    
    obj.module = p.module;
	obj.source = p.source;
	obj.width = p.width;
	obj.height = p.height;
    
    obj.min = (p.min ? p.min : 0);
	obj.max = (p.max ? p.max : 1);
	obj.scale = 1/(obj.max == obj.min ? 1 : obj.max-obj.min);
    
	obj.min_x = (p.min_x ? p.min_x : obj.min);
	obj.max_x = (p.max_x ? p.max_x : obj.max);
	obj.scale_x = 1/(obj.max_x == obj.min_x ? 1 : obj.max_x-obj.min_x);
    
	obj.min_y = (p.min_y ? p.min_y : obj.min);
	obj.max_y = (p.max_y ? p.max_y : obj.max);
	obj.scale_y = 1/(obj.max_y == obj.min_y ? 1 : obj.max_y-obj.min_y);
    
    obj.flip_x_axis = (p.flip_x_axis ? p.flip_x_axis == "yes" : false);
    obj.flip_y_axis = (p.flip_y_axis ? p.flip_y_axis == "yes" : false);

    obj.stroke_LUT = makeLUTArray(p.color, ['yellow']);
    obj.fill_LUT = makeLUTArray(p.fill, ['none']);
	obj.stroke_width = (p.stroke_width ? p.stroke_width : 1);
    obj.line_cap = (p.line_cap ? p.line_cap : "butt");
    obj.line_join = (p.line_join ? p.line_join : "miter");

    var view = document.getElementById("frame");
    
    var r = document.createElement("div");
    r.className = "object_background"
    r.style.left = p.x;
    r.style.top = p.y;
    r.style.width = p.width;
    r.style.height = p.height;
    view.appendChild(r);
    
    this.bg = r;
    
    if(!obj.oversampling)
        obj.oversampling = 1;

    obj.canvas = document.createElement("canvas");
    obj.canvas.style.width = p.width;
    obj.canvas.style.height = p.height;
    obj.canvas.width = obj.oversampling*p.width;
    obj.canvas.height = obj.oversampling*p.height;
    obj.canvas.style.borderRadius = "11px";
    
    this.bg.appendChild(obj.canvas);
    
    obj.context = obj.canvas.getContext("2d");
    
    if(obj.flip_x_axis)
    {
        obj.context.translate(obj.canvas.width, 0);
        obj.context.scale(-1, 1);
    }
    
    if(obj.flip_y_axis)
    {
        obj.context.translate(0, obj.canvas.height);
        obj.context.scale(1, -1);
    }

    obj.context.clearRect(0, 0, obj.width, obj.height);
    obj.context.fillStyle="none";
    obj.context.fillRect(0, 0, obj.width, obj.height);
    obj.context.lineCap = obj.line_cap;
    obj.context.lineJoin = obj.line_join;

	if(!(p.opaque != undefined ? p.opaque=='yes' : p.behind))
    {
        r.style.background="none";
        return;
    }

    var h = document.createElement("div");
    h.className = "object_titlebar"
    h.style.width = p.width;
    r.appendChild(h);
    
    var t = document.createElement("div");
    t.className = "object_title"
    t.style.width = p.width;
    r.appendChild(t);
    
    var tt = document.createTextNode(p.title);
    t.appendChild(tt);
    
    obj.context.drawArrow = function(arrow)
    {
        this.beginPath();
        this.moveTo(arrow[arrow.length-1][0],arrow[arrow.length-1][1]);
        for(var i=0;i<arrow.length;i++){
            this.lineTo(arrow[i][0],arrow[i][1]);
        }
        this.closePath();
        this.fill();
        this.stroke();
    };
    
    obj.context.moveArrow = function(arrow, x, y)
    {
        var rv = [];
        for(var i=0;i<arrow.length;i++){
            rv.push([arrow[i][0]+x, arrow[i][1]+y]);
        }
        return rv;
    };
    
    obj.context.rotateArrow = function(arrow,angle)
    {
        var rv = [];
        for(var i=0; i<arrow.length;i++){
            rv.push([(arrow[i][0] * Math.cos(angle)) - (arrow[i][1] * Math.sin(angle)),
                     (arrow[i][0] * Math.sin(angle)) + (arrow[i][1] * Math.cos(angle))]);
        }
        return rv;
    };
    
    obj.context.drawArrowHead = function(fromX, fromY, toX, toY)
    {
        var angle = Math.atan2(toY-fromY, toX-fromX);
        var arrow = [[0,0], [-10,-5], [-10, 5]];
        this.save();
        this.lineJoin = "miter";
        this.fillStyle = this.strokeStyle;
        this.drawArrow(this.moveArrow(this.rotateArrow(arrow,angle),toX,toY));
        this.restore();
    };
    
    obj.context.drawLineArrow = function(fromX, fromY, toX, toY)
    {
        this.beginPath();
        this.moveTo(fromX,fromY);
        this.lineTo(toX,toY);
        this.stroke();
        var angle = Math.atan2(toY-fromY, toX-fromX);
        var arrow = [[0,0], [-10,-5], [-10, 5]];
        this.drawArrow(this.moveArrow(this.rotateArrow(arrow,angle),toX,toY));
    };

    obj.context.drawLine = function(fromX, fromY, toX, toY)
    {
        this.beginPath();
        this.moveTo(fromX,fromY);
        this.lineTo(toX,toY);
        this.stroke();
    };
}



function Graph(p, title)
{
    this.obj = 	new WebUIObject(this, p, p.module+'.'+p.source);

    this.svg = document.createElementNS(svgns,"svg");
    this.svg.setAttribute('width', p.width);
    this.svg.setAttribute('height', p.height);

    
    this.obj.bg.appendChild(this.svg);
    
    clip_id++;
	this.clip = document.createElementNS(svgns,"clipPath");
	this.clip.setAttribute('id', 'clip'+clip_id);
	this.svg.appendChild(this.clip);
    
	this.clipRect = document.createElementNS(svgns,"rect");
	this.clipRect.setAttribute('x', '0');
	this.clipRect.setAttribute('y', '0');
	this.clipRect.setAttribute('rx', '10');
	this.clipRect.setAttribute('ry', '10');
	this.clipRect.setAttribute('width', p.width);
	this.clipRect.setAttribute('height', p.height);
	this.clip.appendChild(this.clipRect);

    this.group = document.createElementNS(svgns,"g");
	this.group.setAttribute("clip-path", 'url(#clip'+clip_id+')');
	this.group.setAttribute("transform", "translate(+0.5,+0.5)");
    this.svg.appendChild(this.group);
}


Graph.prototype.AddRect = function (x, y, width, height, f, s, radius)
{
    var r = document.createElementNS(svgns,"rect");	
    r.setAttribute('x', x);
    r.setAttribute('y', y);
    r.setAttribute('width', width);
    r.setAttribute('height', height);
    if(f) r.setAttribute('fill', f);
    if(s) r.setAttribute('stroke', s);
    if(radius) r.setAttribute('rx', radius);
    if(radius) r.setAttribute('ry', radius);
    
    this.group.appendChild(r);
    
    return r;
}



Graph.prototype.AddPolygon = function (points, f, s, w)
{
    var r = document.createElementNS(svgns,"polygon");	
    r.setAttribute('points', points);
    if(f) r.setAttribute('fill', f);
    if(s) r.setAttribute('stroke', s);
    if(w) r.setAttribute('stroke-width', w);
    
    this.group.appendChild(r);
    
    return r;
}



Graph.prototype.AddClippingRect= function (x, y, width, height)
{
    clip_id++;
    
    var g = document.createElementNS(svgns,"g");
    
    g.setAttribute('clip-path', 'url(#clip'+clip_id+')');
    
    var cp = document.createElementNS(svgns,"clipPath");
    cp.setAttribute('id', 'clip'+clip_id);
    
    g.appendChild(cp);
    
    cr = document.createElementNS(svgns,"rect");
    cr.setAttribute('x', y);
    cr.setAttribute('y', y);
    cr.setAttribute('width', width);
    cr.setAttribute('height', height);
    
    cp.appendChild(cr);
    
    this.group.appendChild(g);
    
    return g;
}

Graph.prototype.AddCircle = function (cx, cy, radius, f, s, sw)
{
    var r = document.createElementNS(svgns,"circle");	
    r.setAttribute('cx', cx);
    r.setAttribute('cy', cy);
    r.setAttribute('r', radius);
    
    if(f) r.setAttribute('fill', f);
    if(s) r.setAttribute('stroke', s);
    if(sw) r.setAttribute('stroke-width', sw);
    
    this.group.appendChild(r);
    
    return r;
}

Graph.prototype.AddLine = function (x1, y1, x2, y2, s, sw)
{
    var r = document.createElementNS(svgns,"line");	
    r.setAttribute('x1', x1);
    r.setAttribute('y1', y1);
    r.setAttribute('x2', x2);
    r.setAttribute('y2', y2);
    
    if(s) r.setAttribute('stroke', s);
    if(sw) r.setAttribute('stroke-width', sw);
    
    this.group.appendChild(r);
    
    return r;
}



Graph.prototype.AddImage = function (x, y, width, height, path)
{
    var r = document.createElementNS(svgns,"image");	
    r.setAttribute('x', x);
    r.setAttribute('y', y);
    r.setAttribute('width', width);
    r.setAttribute('height', height);
    r.setAttribute('preserveAspectRatio', 'none');
    r.setAttributeNS(xlinkns, 'href', path);
    
    this.group.appendChild(r);
    
    return r;
}



Graph.prototype.AddText = function (x, y, t, width, height, anchor, fontsize, color)
{
    var r = document.createElementNS(svgns,"text");	
    r.setAttribute('x', x);
    r.setAttribute('y', y);
    r.setAttribute('font-size', (fontsize ? fontsize : "11px"));
    r.setAttribute('font-family', 'sans-serif');
    r.setAttribute('fill', (color ? color: '#BABABA'));

    if(anchor) r.setAttribute('text-anchor', anchor);
    r.setAttribute('text-rendering','optimizeLegibility');
    var tn = document.createTextNode(t);	
    r.appendChild(tn);
    this.group.appendChild(r);
    
    return r;
}



Graph.prototype.AddHTMLText = function (x, y, t, width, height, style)
{
    var r = document.createElementNS(svgns,"foreignObject");
    r.setAttribute('x', x);
    r.setAttribute('y', y);
    r.setAttribute('width', width);
    r.setAttribute('height', height);
    
    var b = document.createElementNS(xmlns,"body");	
    b.setAttribute('xmlns', 'http://www.w3.org/1999/xhtml');

    var d = document.createElementNS(xmlns,"div");	
    b.setAttribute('style', (style ? style: 'font-family: sans-serif; font-size: 11px; color: #BABABA'));

    var tn = document.createTextNode(t);	

    d.appendChild(tn);
    b.appendChild(d);
    r.appendChild(b);
    this.group.appendChild(r);
    
    return r;
}



Graph.prototype.AddHTMLPopUp = function (x, y, title, list, width, height, style)
{
    var r = document.createElementNS(svgns,"foreignObject");
    r.setAttribute('x', x);
    r.setAttribute('y', y);
    r.setAttribute('width', width);
    r.setAttribute('height', height);
    
    var b = document.createElementNS(xmlns,"body");
    b.setAttribute('xmlns', 'http://www.w3.org/1999/xhtml');

    var d = document.createElementNS(xmlns,"div");	
    d.setAttribute('style', (style ? style: 'font-family: sans-serif; font-size: 11px; color: white'));
    b.appendChild(d);

    if(title != "")
    {
        var tn = document.createTextNode(title+": ");
        d.appendChild(tn);
    }

    var s = document.createElementNS(xmlns,"select");

    // Add options

    l = list.split("/");
    for(i in l)
    {
        var o = document.createElement("option");
        o.setAttribute("value", i.toString());
        var t = document.createTextNode(l[i]);
        o.appendChild(t);
        s.appendChild(o);
    }

    d.appendChild(s);
    r.appendChild(b);
    this.group.appendChild(r);
    
    return s; // return relevant HTML object
}



Graph.prototype.AddHTMLCanvas = function (x, y, width, height)
{
    var r = document.createElementNS(svgns,"foreignObject");
    r.setAttribute('x', x);
    r.setAttribute('y', y);
    r.setAttribute('width', width);
    r.setAttribute('height', height);
    
    var b = document.createElementNS(xmlns,"body");
    b.setAttribute('xmlns', 'http://www.w3.org/1999/xhtml');
    
    var canvas = document.createElementNS(xmlns, "canvas");
    canvas.setAttribute('width', width);
    canvas.setAttribute('height', height);

    b.appendChild(canvas);
    r.appendChild(b);
    this.group.appendChild(r);
    
    return canvas;
}



var uiobject = new Array(0);



function ignore_data(obj)
{
}


function get(url, callback)
{
	function getURL_alt(url, callback)
	{
		function callCallback()
		{
			if (ajaxRequest.readyState == 4)
			{
				if(ajaxRequest.status == 200)
					if (ajaxCallback) ajaxCallback({content:ajaxRequest.responseText});
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
    
	if(window.getURL)
		getURL(url, callback);
	else if(window.XMLHttpRequest)
		getURL_alt(url, callback);
	else
		alert("Error: This web browser does not have getURL or XMLHttpRequest");
}



var load_count = 0;
var load_count_timeout = null;
var g_data = null;



function update_all()
{
    for(i in uiobject)
    {
        try
        {
            uiobject[i].Update(g_data);
        }
        catch(err)
        {
        // view is being loaded - ignore!
        //   if(console) console.log("Error: "+err.message);
        //   alert("Exception");
        }
    }
}



function clear_wait()
{
    load_count = 0;
}



function wait_for_load(data)
{
    if(load_count > 0)
        setTimeout("wait_for_load()", 1);    
    else
    {
        clearTimeout(load_count_timeout);
        update_all();
    }
}



function update(data)
{
    load_count = 0;
    g_data = data;

    try
    {
        for(i in uiobject)
        {
            if(uiobject[i].LoadData)
            {
                load_count += uiobject[i].LoadData(data);
            }
        }
        
        load_count_timeout = setTimeout("clear_wait()", 200); // give up after 1/5 s and continue
        setTimeout("wait_for_load()", 1);
    }
    catch(err)
    {
        //     view is being loaded - ignore!
        //        if(console) console.log("Error: "+err.message);
        //        alert("Exception");
    }
}



function add(obj)
{
	uiobject.push(obj);
    
}



function usesData(module, source)
{
    if(module && source)
        get("/uses/"+module+'/'+source, ignore_data);
}



function usesBase64Data(module, source, type)
{
    if(module && source)
        get("/usesBase64/"+module+'/'+source+'/'+type, ignore_data);
}


