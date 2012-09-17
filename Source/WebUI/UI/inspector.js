function toggle_parameter_info(e)
{
    var t = e.parentElement.children.item(1);
    var c = t.getAttribute("class");
    if(c == "hidden")
        t.setAttribute("class","visible");
    else
        t.setAttribute("class","hidden");
}


function toggle_class_info(e)
{
    var t = e.parentElement.children.item(2);
    var c = t.getAttribute("class");
    if(c == "hidden")
        t.setAttribute("class","visible");
    else
        t.setAttribute("class","hidden");
}