var ikc_doc_element = null;
var current_group_path = "";
var timer = null;
var periodic_task = null;
var current_view = 0;
var view_list = [];

// PROFILING DATA

var start_time = 0;
var profiling = { stop_after: 0, parsing: 0, image_decoding: 0, image_drawing: 0, view_update: 0, all: 0, total: 0 };

function show_profiling_data()
{
    document.write("<html>");
    document.write("<style>");
    document.write("td { border: 1px solid gray; text-align:center; width: 100px; font-family: sans-serif; font-size: 10pt; width: 80px}");
    document.write("</style>");
    document.write("<body style='font-family: sans-serif; font-size: 10pt;'>");
    document.write("<h1>WebUI Profiling Results</h1>");
    document.write("<table style='border-collapse: collapse'>");

    document.write("<tr><td>no of ticks</td><td>JSON<br />parsing<br />(ms)</td><td>image<br />decoding<br />(ms)</td><td>image<br />drawing<br />(ms)</td><td>update<br />view<br />(ms)</td><td>all <br />JavaScript<br />(ms)</td><td>total/tick<br />(ms)</td></tr>");
    document.write("<tr><td>"+profiling.stop_after+"</td><td>"+profiling.parsing/profiling.stop_after+"</td><td>"+profiling.image_decoding/profiling.stop_after+"</td><td>"+profiling.image_drawing/profiling.stop_after+"</td><td>"+profiling.view_update/profiling.stop_after+"</td><td>"+profiling.all/profiling.stop_after+"</td><td>"+profiling.total/profiling.stop_after+"</td></tr>");
    
    document.write("</table>");
    document.write("</body></html>");
}



// STATE

var realtime = false;
var reconnect_counter = 0;


// COOKIES FOR PERSISTENT STATE

function setCookie(name,value,days)
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



// UTILITIES

function ignore_data(obj)
{
}



function get(url, callback, timeout, time)
{
    var ajaxRequest = null;
    var ajaxTimeout = null;
    
    ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open("GET", url, true);
    ajaxRequest.setRequestHeader("Cache-Control", "no-cache");
    ajaxRequest.onload = function () { clearTimeout(ajaxTimeout); callback({content:ajaxRequest.responseText}); };
    ajaxTimeout = setTimeout(function () { ajaxRequest.abort(); if(timeout) timeout(); }, 1000);
    ajaxRequest.send();
}



function getXML(url, callback)
{
    var ajaxRequest = null;
    ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open("GET", url, true);
    ajaxRequest.onload = function () { if(callback) callback(ajaxRequest.responseXML); };
    ajaxRequest.send();
}




// CONTROL AND USER INTERFACE

function select_button(i)
{
	var c = document.getElementById("controls");
    for(var j=0; j<c.children.length; j++)
        c.children.item(j).setAttribute("class","button");
    if(c.children.item(i))
        c.children.item(i).setAttribute("class","selected");
}



function handle_reconnection()
{
    window.location.href=window.location.href;
}



function poll_reconnect()
{
	clearTimeout(periodic_task);

    reconnect_counter++;
    if(reconnect_counter % 2 == 0)
        document.getElementById("iteration").innerText = "◐"; //"◐";
    else
        document.getElementById("iteration").innerText = "◑"; //"◑";

    get("update", handle_reconnection, poll_reconnect, 500);
}



function do_stop()
{
	function handle_data_object(obj)
	{
        select_button(-1);
        document.getElementById("iteration").innerText = ".";
        poll_reconnect();
	}

	clearTimeout(periodic_task);	
	running = false;
    realtime = false;
    
	get("stop", handle_data_object);
}



function do_pause()
{
	function handle_data_object(obj)
	{
        try
        {
            var data = eval("("+obj.content+")");
            var v = document.getElementById("view").contentWindow;
            if(data && v && v.update) v.update(data);
            document.getElementById("iteration").innerText = data.iteration;
        }
        catch(err)
        {
            //alert("Error: "+err.message);
        }
	}

	clearTimeout(periodic_task);	
    select_button(1);
	running = false;
    realtime = false;
    
	get("pause", handle_data_object);
}



function step()
{
	function handle_data_object(obj)
	{
        try
        {
            var data = eval("("+obj.content+")");
            var v = document.getElementById("view").contentWindow;
            if(data && v && v.update) v.update(data);
            document.getElementById("iteration").innerText = data.iteration;
        }
        catch(err)
        {
            //alert("Error: "+err.message);
        }
	}

    get("step", handle_data_object);
}



function do_step()
{
	clearTimeout(periodic_task);
    select_button(2);
	running = false;
    realtime = false;
	step();
}



function runstep()
{
    if(!running)
        return;

    function handle_data_object(obj)
    {
        try
        {
            var d1 = new Date();
            var data = eval("("+obj.content+")");
            var d2 = new Date();
            profiling.parsing += (d2.getTime()-d1.getTime());
            var v = document.getElementById("view").contentWindow;
            var d3 = new Date();
            if(data && v && v.update) v.update(data);
            var d4 = new Date();
            profiling.view_update += (d4.getTime()-d3.getTime());
            document.getElementById("iteration").innerText = data.iteration;
            
            if(data.iteration == profiling.stop_after)
            {
                var stop_time = new Date();
                profiling.total = (stop_time.getTime()-start_time.getTime());
                do_stop();
                show_profiling_data();
                return;
            }

            if(running)
                setTimeout("runstep();", 1);
            var d5 = new Date();
            profiling.all += (d5.getTime()-d1.getTime());
        }
        catch(err)
        {
            // Connection broken!!!
            poll_reconnect();
        }
    }

    get("runstep", handle_data_object, poll_reconnect, 1000);
}



function do_run()
{
    start_time = new Date();
	clearTimeout(periodic_task);    
    select_button(3);
	running = true;
    realtime = false;
    runstep();
}



function update()
{
	function handle_data_object(obj)
	{
        try
        {
            var data = eval("("+obj.content+")");
            var v = document.getElementById("view").contentWindow;
            if(data && v && v.update) v.update(data);
            document.getElementById("iteration").innerText = data.iteration;
        
            if(data.state == 3)
                do_run();
            else if(data.state == 4)
                setTimeout("update();", 100);
        }
        catch(err)
        {
            poll_reconnect();
        }
	}

	get("update", handle_data_object, poll_reconnect, 1000);
}



function do_update()
{
	var b = document.getElementsByTagName("image");
	update();
}



function do_realtime()
{
	clearTimeout(periodic_task);
    select_button(4);
	running = false;
    realtime = true;
	get("realtime", ignore_data);
    setTimeout("update();", 10);
}



function getChildrenByTagName(element, name)
{
    if(!element)
        return null;
    
    if(!element.childNodes)
        return null;
    
    var list = new Array();
    for(var i=0; i<element.childNodes.length; i++)
        if (element.childNodes[i].nodeName==name)
            list.push(element.childNodes[i]);
            
    return list;
}



function getGroupWithName(element, name)
{
    if(!element)
        return null;
    
    if(!element.childNodes)
        return null;

    for(var i=0; i<element.childNodes.length; i++)
        if (element.childNodes[i].nodeName=="group" &&  element.childNodes[i].getAttribute("name")==name)
            return element.childNodes[i];

    return null;
}



function build_group_list(group, list, p, top, selected_element)
{
    if(!group)
        return;

    if(!list)
        return;

    for(i in group)
    {
        var name = group[i].getAttribute("title");
        if(!name)
            name = group[i].getAttribute("name");
        if(!name)
            name = "Untitled";


        var ip= (top ? "" : p + "/" + name);

        var subgroups = getChildrenByTagName(group[i], "group");

        var li = document.createElement("li");
        var bar  = document.createElement("span"); 
        var tri_span = document.createElement("span");
        var span = document.createElement("span");

        var triangle;
        tri_span.setAttribute("class","group-closed");
        tri_span.path = (top ? "" : ip);

        bar.appendChild(tri_span);

        var txt = document.createTextNode(name);
        span.appendChild(txt);
        span.path = (top ? "" : ip);
        
        if(top && !selected_element)
        {
            bar.setAttribute("class","group-bar-selected");
            span.setAttribute("class","group-selected");
        }
        else if(top)
        {
            bar.setAttribute("class","group-bar");
            span.setAttribute("class","group-unselected");
            tri_span.setAttribute("class","group-open");
        }
        else if(selected_element && name == selected_element[0])
        {
            bar.setAttribute("class","group-bar-selected");
            span.setAttribute("class","group-selected");
        }
        else
        {
            bar.setAttribute("class","group-bar");
            span.setAttribute("class","group-unselected");
        }

        bar.appendChild(span);

        li.appendChild(bar);

        if(subgroups.length>0)
        {
            if(tri_span.class_name=="group-closed")
            {
                triangle = document.createTextNode("▷ ");
                tri_span.appendChild(triangle);
            }
            else
            {
                triangle = document.createTextNode("▽ ");
                tri_span.appendChild(triangle);
            }
            var ul = document.createElement("ul");
            ul.setAttribute("class", "hidden");
            if(selected_element) selected_element.shift();
            build_group_list(subgroups, ul, ip, null, selected_element);
            li.appendChild(ul);
        }

        list.appendChild(li);
    }
}



function build_view_list_with_editor(view)
{
    if(!view)
        return;
    
    view_list = view;

    var vc = document.getElementById("viewdots");
 
    if(!vc)
        return;
    
    while(vc.firstChild)
        vc.removeChild(vc.firstChild);

    for(i in view_list)
    {
        var a = document.createElement("a");
        var dot = document.createTextNode("○ ");
        a.appendChild(dot);
        vc.appendChild(a);
        
        a.setAttribute("onclick","change_view("+i+")");
        a.setAttribute("title",view_list[i].getAttribute("name"));
    }
    
    // add edit view
    
    var a = document.createElement("a");
    var dot = document.createTextNode("◽ ");
    a.appendChild(dot);
    vc.appendChild(a);
    a.setAttribute("onclick","change_view("+view_list.length+")");
    a.setAttribute("title","Inspector");

    // add inspector view
    
    var a = document.createElement("a");
    var dot = document.createTextNode("ℹ ");
    a.appendChild(dot);
    a.style.lineHeight="80%";
    vc.appendChild(a);
        
    a.setAttribute("onclick","change_view("+(view_list.length+1)+")");
    a.setAttribute("title","Editor");
    
    // TODO: Check that this is ok to remove
    //change_view(1);
}


function build_view_list(view)
{
    if(!view)
        return;
    
    view_list = view;

    var vc = document.getElementById("viewdots");
 
    if(!vc)
        return;
    
    while(vc.firstChild)
        vc.removeChild(vc.firstChild);

    for(i in view_list)
    {
        var a = document.createElement("a");
        var dot = document.createTextNode("○ ");
        a.appendChild(dot);
        vc.appendChild(a);
        
        a.setAttribute("onclick","change_view("+i+")");
        a.setAttribute("title",view_list[i].getAttribute("name"));
    }
    
    // add inspector view
    
    var a = document.createElement("a");
    var dot = document.createTextNode("ℹ ");
    a.appendChild(dot);
    a.style.lineHeight="80%";
    vc.appendChild(a);
        
    a.setAttribute("onclick","change_view("+(view_list.length)+")");
    a.setAttribute("title","Editor");
    
    // TODO: Check this is ok
    //change_view(1);
}



function change_view_with_editor(index)
{
    current_view = index;

    var vc = document.getElementById("viewdots");
    var alist = getChildrenByTagName(vc, "A");
    for(i in alist)
        if(i == alist.length-2)
            alist[i].innerHTML = (i == index ? "ℹ " : "ℹ ");
        else if(i==alist.length-1)
            alist[i].innerHTML = (i == index ? "◾ " : "◽ ");
        else
            alist[i].innerHTML = (i == index ? "● " : "○ ");
        
    var v = document.getElementById("view");
    if(!v) return;
    
    if(index==alist.length-2)
        v.setAttribute("src", "http://"+location.host+current_group_path+"/inspector.html");    //FIXME:  change order
    else if(index==alist.length-1)
        v.setAttribute("src", "http://"+location.host+current_group_path+"/editor.svg");        //FIXME:  change order
    else
        v.setAttribute("src", "http://"+location.host+"/view"+current_group_path+"/view"+current_view+".svg");
    
    var vn = document.getElementById("viewname");
    if(vn)
    {        
        if(index==alist.length-2)
            vn.innerHTML = "Inspector"
        else if(index==alist.length-1)
            vn.innerHTML = "Editor"
        else
        {
            var vw = view_list[current_view];
            if(vw)
                vn.innerHTML = vw.getAttribute("name");
            else
                vn.innerHTML = "View "+index;
        }
    }
}



function change_view(index)
{
    current_view = index;
    setCookie('current_view', current_view);

    var vc = document.getElementById("viewdots");
    var alist = getChildrenByTagName(vc, "A");
    for(i in alist)
        if(i == alist.length-1)
            alist[i].innerHTML = (i == index ? "ℹ " : "ℹ ");
        else
            alist[i].innerHTML = (i == index ? "● " : "○ ");
        
    var v = document.getElementById("view");
    if(!v) return;
    
    if(index==alist.length-1)
        v.setAttribute("src", "http://"+location.host+current_group_path+"/inspector.html");    //FIXME:  change order
    else
        v.setAttribute("src", "http://"+location.host+"/view"+current_group_path+"/view"+current_view+".html"); // WAS SVG ***************************
    
    var vn = document.getElementById("viewname");
    if(vn)
    {        
        if(index==alist.length-1)
            vn.innerHTML = "Inspector"
        else
        {
            var vw = view_list[current_view];
            if(vw)
                vn.innerHTML = vw.getAttribute("name");
            else
                vn.innerHTML = "View "+index;
        }
    }
}



function restore_view()
{
    var v = getCookie('current_view');
    change_view(v?parseInt(v):0);
}



function next_view()
{
    if(current_view < view_list.length+1)
        change_view(current_view+1);
}



function prev_view()
{
    if(current_view > 0)
        change_view(current_view-1);
}



function update_group_list_and_views()
{
	function handle_data_object(xml)
	{
        try
        {
            ikc_doc_element = xml.documentElement;
            var title = document.getElementById("title");
            
            var name = ikc_doc_element.getAttribute("title");
            if(!name)
                name = ikc_doc_element.getAttribute("name");
            if(!name)
                name = "Untitled";

            if(title) title.innerText = name;
            
            grouplist = document.getElementById("grouplist");
            element_list = getCookie('root');
            build_group_list([xml.documentElement], grouplist, "", true, (element_list?element_list.split('/'):null));
            build_view_list(getChildrenByTagName(xml.documentElement, "view"));
            // change_view(0);
            restore_view();
        }
        catch(err)
        {
            alert("Error in update_group_list_and_views. XML could not be parsed (possibly because of an unknown entitiy): "+err.message);
        }
	}

	getXML("xml.ikc", handle_data_object);
}



function toggle_inspector()
{
	if(document.getElementById('pane').style.width == "0px")
    {
        document.getElementById('pane').style.width='300px';
        document.getElementById('split').src="/Icons/single.png";
        setCookie('inspector','open');
	}
    else
	{
        document.getElementById('pane').style.width='0px';
        document.getElementById('split').src="/Icons/split.png";
        setCookie('inspector','closed');
    }
}



function restore_inspector()
{
    if(getCookie('inspector') == 'open')
    {
        document.getElementById('pane').style.width='300px';
        document.getElementById('split').src="/Icons/single.png";
	}
    else
	{
        document.getElementById('pane').style.width='0px';
        document.getElementById('split').src="/Icons/split.png";
    }
}



function toggle(e) // TODO: do something smarter than selecting first view on toggle?
{
    
	if(e.target.getAttribute("class") == "group-open")
    {
		e.target.setAttribute('class', 'group-closed');
        e.target.innerText = "▷ ";

        var ul = e.target.parentElement.parentElement.children.item(1);
        if(ul)
            ul.setAttribute('class', 'hidden');
    }
    
	else if(e.target.getAttribute("class") == "group-closed")
    {
		e.target.setAttribute('class', 'group-open');
        e.target.innerText = "▽ ";

        var ul = e.target.parentElement.parentElement.children.item(1);
        if(ul)
            ul.setAttribute('class', 'visible');
    }
    
    else if(e.target.getAttribute("class") == "group-unselected")
    {
        var s = document.getElementsByClassName("group-selected");
        for(var i=0; i<s.length; i++)
        {
            var si = s.item(i);
            si.setAttribute("class", "group-unselected");
            si.parentElement.setAttribute("class", "group-bar");
        }
        
		e.target.setAttribute("class", "group-selected");
		e.target.parentElement.setAttribute("class", "group-bar-selected");

        current_group_path = e.target.path;
        get("/setroot"+current_group_path, ignore_data);
        
        setCookie('root', current_group_path);
        
        var p = e.target.path.split('/');
        p.shift();
        var e = ikc_doc_element;
        for(i in p)
            e = getGroupWithName(e, p[i]);   
        build_view_list(getChildrenByTagName(e, "view"));
        change_view(0); // TODO: HERE!!!
    }

	if (e.stopPropagation) e.stopPropagation();
}



function restore_root()
{
    alert("Root: "+getCookie('root'));
}



// Call after load

var grouplist = document.getElementById("grouplist");
grouplist.addEventListener('click', toggle, false);

update_group_list_and_views();

// Restor state after reload

restore_inspector();
restore_view();
//restore_root();

update();

do_run();
