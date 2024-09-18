class WebUIWidgetButton extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "BUTTON", 'control':'header'},

            {'name':'title', 'default':"Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"Button", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'background', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'icon', 'default':"", 'type':'string', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},

            { 'name': 'type', 'default': "push", 'type': 'string', 'control': 'menu', 'options': "push,toggle,radio,multi,input,open" },
            { 'name':'radioGroup', 'default':"", 'type':'string', 'control': 'textedit'},
            { 'name':'multiGroup', 'default':"", 'type':'string', 'control': 'textedit'},


            {'name':'command', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'commandUp', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            
            {'name':'file_names', 'default':"", 'type':'source', 'control': 'textedit'},
        
//            {'name':'state', 'default':"0", 'type':'int', 'control': 'textedit'},
            {'name':'value', 'default':1, 'type':'string', 'control': 'textedit'},
            {'name':'valueUp', 'default':0, 'type':'string', 'control': 'textedit'},
 
            {'name':'xindex', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'yindex', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'enableSource', 'default':"", 'type':'source', 'control': 'textedit'},
                
            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<button type='button class=''></button>";
    }



    requestData(data_set)
    {
        data_set.add(this.parameters.parameter);
        if(this.parameters.file_names)
        data_set.add(this.parameters.file_names);
        if(this.parameters.enableSource)
            data_set.add(this.parameters.enableSource);
    }



    button_down(evt)
    {
        let thisbutton = this;
        let p = this.parentElement.parameters;



        if(p.type == "push")
        {
            if(p.parameter)
                this.parentElement.send_control_change(p.parameter, p.value, p.xindex, p.yindex);
            if(p.command)
                this.parentElement.send_command(p.command, p.value, p.xindex, p.yindex);
        }


        else if(p.type=="toggle")
        {
            if(this.getAttribute("class") != "button-selected")
            {
                this.setAttribute("class","button-selected");
                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.value, p.xindex, p.yindex);
                if(p.command)
                    this.parentElement.send_command(p.command, p.value, p.xindex, p.yindex);

            }
            else
            {
                this.setAttribute("class","");
                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.valueUp, p.xindex, p.yindex);
                if(p.commandUp)
                    this.parentElement.send_command(p.commandUp, p.value, p.xindex, p.yindex);

            }
        }



            else if(p.type=="radio")
            {
                let buttons = document.getElementsByTagName("webui-widget-button");
                for(let b of buttons)
                {
                    if(b.parameters.radioGroup == p.radioGroup && b.firstChild.getAttribute("class")=="button-selected")
                    {
                        b.firstChild.setAttribute("class", "")
                        let q = b.parameters;
                        if(q.parameter)
                            this.parentElement.send_control_change(q.parameter, q.valueUp, q.xindex, q.yindex);
                    }
                }
                this.setAttribute("class", "button-selected")

                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.value, p.xindex, p.yindex);
                if(p.command)
                    this.parentElement.send_command(p.command, p.value, p.xindex, p.yindex);
            }

            else if(p.type=="multi")
            {
                let buttons = document.getElementsByTagName("webui-widget-button");
                for(let b of buttons)
                {
                    if(b.parameters.multiGroup == p.multiGroup)
                        if(b.firstElementChild!=this)
                        {
                            b.firstElementChild.dispatchEvent(new Event('mousedown'));
                        }
                }
            }
            else if(p.type=="input")
            {
                let text = prompt(p.title);
                if(text)
                    this.parentElement.send_command(p.command, text, p.xindex, p.yindex);
            }


            else if(p.type=="open")
            {
                let callback = function (selected_item)
                {
                    thisbutton.parentElement.send_command(p.command, selected_item, p.xindex, p.yindex);
                }

                dialog.showOpenDialog(this.file_names, callback, p.title);
            }
    }



    button_up(evt)
    {
        let p = this.parentElement.parameters;

        if(p.type == "push")
        {
            if(p.parameter)
                this.parentElement.send_control_change(p.parameter, p.valueUp, p.xindex, p.yindex);
            if(p.commandUp)
                this.parentElement.send_command(p.commandUp, p.value, p.xindex, p.yindex);
        }

        else if(p.type=="toggle")
        {

        }

        else if(p.type=="radio")
        {

        }
    }

    init()
    {
        super.init();
        this.firstChild.addEventListener("mousedown", this.button_down, true);
        this.firstChild.addEventListener("mouseup", this.button_up, true);
    }

    update(d)
    {
        if(this.parameters.color)
            this.firstChild.style.color = this.parameters.color;

        if(this.parameters.background)
            this.firstChild.style.background = this.parameters.background;

        if(this.parameters.file_names)
            this.firstChild.file_names = this.getSource("file_names");
        if(this.parameters.icon)
            this.firstChild.innerHTML = "<img src='"+this.parameters.icon+"' class='button-icon'>"; // ' style='width: 70%;height: 70%;object-fit: contain;'
        else
            this.firstChild.innerText = this.parameters.label;

        try
        {
            if(this.parameters.enableSource)
                this.firstChild.disabled = (this.getSource('enableSource')[0][0] == 0 ? true : false)
        }
        catch(err)
        {}
        try {
                if(this.parameters.parameter)
                {
                    let value = this.parameters.value;                   
                    let v = this.getSource('parameter')[this.parameters.yindex][this.parameters.xindex]

                    if(v == value)

                        this.firstChild.setAttribute("class", "button-selected")
                    else
                        this.firstChild.setAttribute("class", "")
                }

        }
        catch(err)
        {}
    }

}



webui_widgets.add('webui-widget-button', WebUIWidgetButton);

