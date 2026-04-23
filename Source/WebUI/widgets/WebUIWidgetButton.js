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
            {'name':'button_background', 'default':"", 'type':'string', 'control': 'textedit'},
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
 
            {'name':'select_x', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'select_y', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'enableSource', 'default':"", 'type':'source', 'control': 'textedit'},
        ]};

    static html()
    {
        return "<button type='button class=''></button>";
    }



    requestData(data_set)
    {
        this.addSource(data_set, this.parameters.parameter);
        if(this.parameters.file_names)
            this.addSource(data_set, this.parameters.file_names);
        if(this.parameters.enableSource)
            this.addSource(data_set, this.parameters.enableSource);
    }

    getSelectX()
    {
        if(this.parameters.select_x !== undefined && this.parameters.select_x !== "")
            return Number(this.parameters.select_x);
        return Number(this.parameters.xindex ?? 0);
    }

    getSelectY()
    {
        if(this.parameters.select_y !== undefined && this.parameters.select_y !== "")
            return Math.trunc(Number(this.parameters.select_y));
        return Math.trunc(Number(this.parameters.yindex ?? 0));
    }

    getButtonBackground()
    {
        if(this.parameters.button_background !== undefined && this.parameters.button_background !== "")
            return this.parameters.button_background;
        return this.parameters.background ?? "";
    }

    usesLegacyButtonBackground()
    {
        return (
            (this.parameters.button_background === undefined || this.parameters.button_background === "") &&
            this.parameters.background !== undefined &&
            this.parameters.background !== ""
        );
    }



    button_down(evt)
    {
        console.log("button down");
        if(!main.edit_mode)
            evt.stopPropagation();
        let thisbutton = this;
        let p = this.parentElement.parameters;
        const selectX = this.parentElement.getSelectX();
        const selectY = this.parentElement.getSelectY();

        if(p.type == "push")
        {
            if(p.parameter)
                this.parentElement.send_control_change(p.parameter, p.value, selectX, selectY);
            if(p.command)
                this.parentElement.send_command(p.command, p.value, selectX, selectY);
        }

        else if(p.type=="toggle")
        {
            if(this.getAttribute("class") != "button-selected")
            {
                this.setAttribute("class","button-selected");
                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.value, selectX, selectY);
                if(p.command)
                    this.parentElement.send_command(p.command, p.value, selectX, selectY);

            }
            else
            {
                this.setAttribute("class","");
                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.valueUp, selectX, selectY);
                if(p.commandUp)
                    this.parentElement.send_command(p.commandUp, p.valueUp, selectX, selectY);

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
                            this.parentElement.send_control_change(q.parameter, q.valueUp, b.getSelectX(), b.getSelectY());
                    }
                }
                this.setAttribute("class", "button-selected")

                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.value, selectX, selectY);
                if(p.command)
                    this.parentElement.send_command(p.command, p.value, selectX, selectY);
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
                    this.parentElement.send_command(p.command, text, selectX, selectY);
            }


            else if(p.type=="open")
            {
                if(main.edit_mode)
                    return; // TEMPORARY

                let callback = function (selected_item)
                {
                    thisbutton.parentElement.send_command(p.command, selected_item, selectX, selectY);
                }

                if(this.file_names)
                    dialog.showListSelectDialog(this.file_names, callback, p.title);
                else
                    dialog.showListSelectDialog("", callback, p.title);
            }
    }



    button_up(evt)
    {
        console.log("button up");
        evt.stopPropagation();
        if(main.edit_mode)
            return;
        let p = this.parentElement.parameters;
        const selectX = this.parentElement.getSelectX();
        const selectY = this.parentElement.getSelectY();

        if(p.type == "push")
        {
            if(p.parameter)
            {
                this.parentElement.send_control_change(p.parameter, p.valueUp, selectX, selectY);
            }
            if(p.commandUp)
                this.parentElement.send_command(p.commandUp, p.valueUp, selectX, selectY);
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
        this.firstChild.addEventListener('click', e => {
            console.log("button click");
            if(main.edit_mode)
                return; 
            e.stopPropagation();
            }, true);
    }

    update(d)
    {
        this.parameters.select_x = this.getSelectX();
        this.parameters.select_y = this.getSelectY();

        if(this.parameters.color)
            this.firstChild.style.color = this.parameters.color;

        const buttonBackground = this.getButtonBackground();
        this.firstChild.style.background = buttonBackground || "";
        if(this.usesLegacyButtonBackground())
            this.parentElement.style.background = "";

        if(this.parameters.file_names)
            this.firstChild.file_names = this.getSource("file_names");
        if(this.parameters.icon)
            this.firstChild.innerHTML = "<img src='"+this.parameters.icon+"' class='button-icon'>"; // ' style='width: 70%;height: 70%;object-fit: contain;'
        else
            this.firstChild.innerText = this.parameters.label;

        try
        {
            if(this.parameters.enableSource)
            {
                const enableSource = this.getSource('enableSource');
                const enableValue = Array.isArray(enableSource) ? (Array.isArray(enableSource[0]) ? enableSource[0][0] : enableSource[0]) : enableSource;
                this.firstChild.disabled = (enableValue == 0 ? true : false);
            }
        }
        catch(err)
        {}
        try {
                if(this.parameters.parameter)
                {
                    let value = this.parameters.value;                   
                    const parameterSource = this.getSource('parameter');
                    if(!Array.isArray(parameterSource))
                        return;
                    const matrix = Array.isArray(parameterSource[0]) ? parameterSource : [parameterSource];
                    if(!Array.isArray(matrix[this.parameters.select_y]))
                        return;
                    let v = matrix[this.parameters.select_y][this.parameters.select_x]

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
