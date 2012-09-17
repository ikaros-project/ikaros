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
    
    if(c.indexOf("LUT_") == 0)
        return eval(c);
    
    return c.split(",");
}



function Graph(p, title)
{
	clip_id++;
	this.clip = document.createElementNS(svgns,"clipPath");
	this.clip.setAttribute('id', 'clip'+clip_id);
	document.documentElement.appendChild(this.clip);
    
	this.clipRect = document.createElementNS(svgns,"rect");
	this.clipRect.setAttribute('x', '0');
	this.clipRect.setAttribute('y', '0');
	this.clipRect.setAttribute('rx', '10');
	this.clipRect.setAttribute('ry', '10');
	this.clipRect.setAttribute('width', p.width);
	this.clipRect.setAttribute('height', p.height);
	this.clip.appendChild(this.clipRect);
    
	if(!p.title)
		p.title = title;
    
    // Create outer group
    
    this.main = document.createElementNS(svgns,"g");
	this.main.setAttribute("clip-path", 'url(#clip'+clip_id+')');
	this.main.setAttribute("transform", "translate("+p.x+","+p.y+")");
    
    // Add contents group
    
    this.group = document.createElementNS(svgns,"g");
    
	if(p.opaque != undefined ? p.opaque=='yes' : p.behind)
	{
		this.background = this.AddRect(0, 0, p.width-1, p.height-1);
        this.background.setAttribute("class", "object_background");
        //        this.background.setAttribute("filter", "url(#filter)");
	}
    
	this.main.appendChild(this.group);
    
    // Add title bar
    
	this.titlebar = document.createElementNS(svgns,"g");
    
	if(p.opaque != undefined ? p.opaque=='yes' : p.behind)
	{
        this.dragarea = document.createElementNS(svgns,"rect");	
        this.dragarea.setAttribute('x', 0);
        this.dragarea.setAttribute('y', 0);
        this.dragarea.setAttribute('width', p.width-1);
        this.dragarea.setAttribute('height', 20+1);
        this.dragarea.setAttribute('class','object_titlebar');
        this.titlebar.appendChild(this.dragarea);
        
        this.title = document.createElementNS(svgns,"text");	
        this.title.setAttribute('x', 10);
        this.title.setAttribute('y', 14);
        this.title.setAttribute('class', 'object_title');        
        this.title.setAttribute('text-rendering','optimizeLegibility');
        var tn = document.createTextNode(p.title);	
        this.title.appendChild(tn);
        this.titlebar.appendChild(this.title);
	}
    
	this.main.appendChild(this.titlebar);
    
    document.documentElement.appendChild(this.main);
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



function update(data)
{
    try {
        for(i in uiobject)
        {
            uiobject[i].Update(data);
        }
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
	get("/uses/"+module+'/'+source, ignore_data);
}

