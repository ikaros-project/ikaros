var ikc_doc_element = null;
var current_group_path = "";
var timer = null;
var periodic_task = null;
var current_view = 0;
var view_list = [];

// STATE

var realtime = false;
var reconnect_counter = 0;


// UTILITIES

function ignore_data(obj)
{
}



function get(url, callback, timeout, time)
{
	function getURL_alt(url, callback)
	{
        function timoutCallback()
        {
            if(timeout) timeout();
        }

		function callCallback()
		{
			if (ajaxRequest.readyState == 4)
			{
				if(ajaxRequest.status == 200)
                {
                    clearTimeout(ajaxTimeout);
                    if (ajaxCallback) ajaxCallback({content:ajaxRequest.responseText});
                }
				//else
				//	alert("Returned status code " + ajaxRequest.status);
			}
		}

		var ajaxRequest = null;
		var ajaxCallback = callback;

		ajaxRequest = new XMLHttpRequest();
		ajaxRequest.onreadystatechange = callCallback;
		ajaxRequest.open("GET", url, true);
		ajaxRequest.send(null);
        
        if(timeout)
            ajaxTimeout = setTimeout(function () { timoutCallback(); }, time);
	}			

	if(window.getURL)
		getURL(url, callback);
	else if(window.XMLHttpRequest)
		getURL_alt(url, callback);
	else
		alert("Error: This web browser does not have getURL or XMLHttpRequest");
}



function getXML(url, callback)
{
	function getURL_alt(url, callback)
	{
		function callCallback()
		{
			if (ajaxRequest.readyState == 4)
			{   
				if(ajaxRequest.status == 200)
					if (ajaxCallback) ajaxCallback(ajaxRequest.responseXML);
			}
            else
            {
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
    return; // FIXME: problems problems

    reconnect_counter++;
    if(reconnect_counter % 2 == 0)
        document.getElementById("iteration").innerText = "."; //"◐";
    else
        document.getElementById("iteration").innerText = " "; //"◑";

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
    
    // TODO: wait for all images to load!!!

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
            var data = eval("("+obj.content+")");
            var v = document.getElementById("view").contentWindow;
            if(data && v && v.update) v.update(data);
            document.getElementById("iteration").innerText = data.iteration;
            if(running)
                setTimeout("runstep();", 10);
        }
        catch(err)
        {
            //alert("Error: "+err.message);
        }
    }

    get("runstep", handle_data_object, poll_reconnect, 1000);
}


function do_run()
{

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
                setTimeout("update();", 10);
        }
        catch(err)
        {
            //alert("Error: "+err.message);
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



function build_group_list(group, list, p, top)
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
        
        if(top)
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
            triangle = document.createTextNode("▷ ");
            tri_span.appendChild(triangle);

            var ul = document.createElement("ul");
            ul.setAttribute("class", "hidden");
            build_group_list(subgroups, ul, ip);
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
    
    change_view(1);
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
    
    change_view(1);
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
        v.setAttribute("src", "http://"+location.host+"/view"+current_group_path+"/view"+current_view+".svg");
    
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
            build_group_list([xml.documentElement], grouplist, "", true);
            build_view_list(getChildrenByTagName(xml.documentElement, "view"));
            change_view(0);
        }
        catch(err)
        {
            alert("Error in update_group_list_and_views: "+err.message);
            // RETRY
        }
	}

	getXML("xml.ikc", handle_data_object);
}



function toggle_inspector()
{
	if(document.getElementById('pane').style.width == "0px")
    {
        document.getElementById('pane').style.width='300px';
        document.getElementById('split').src="single.png";
	}
    else
	{
        document.getElementById('pane').style.width='0px';
        document.getElementById('split').src="split.png";
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
        var p = e.target.path.split('/');
        p.shift();
        var e = ikc_doc_element;
        for(i in p)
            e = getGroupWithName(e, p[i]);   
        build_view_list(getChildrenByTagName(e, "view"));
        change_view(0);
    }

	if (e.stopPropagation) e.stopPropagation();
}



// Call after load

var grouplist = document.getElementById("grouplist");
grouplist.addEventListener('click', toggle, false);

update_group_list_and_views();
update();


