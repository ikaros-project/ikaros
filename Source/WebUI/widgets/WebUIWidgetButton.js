class WebUIWidgetButton extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "BUTTON", 'control':'header'},

            {'name':'title', 'default':"Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"Button", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'text_color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'background_color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'icon', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'tooltip', 'default':"", 'type':'string', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},

            { 'name': 'type', 'default': "push", 'type': 'string', 'control': 'menu', 'options': "push,toggle,radio,multi,input,open" },
            { 'name':'radio_group', 'default':"", 'type':'string', 'control': 'textedit'},
            { 'name':'multi_group', 'default':"", 'type':'string', 'control': 'textedit'},


            {'name':'command', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'release_command', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            
            {'name':'file_names_source', 'default':"", 'type':'source', 'control': 'textedit'},
        
//            {'name':'state', 'default':"0", 'type':'int', 'control': 'textedit'},
            {'name':'value', 'default':1, 'type':'string', 'control': 'textedit'},
            {'name':'release_value', 'default':0, 'type':'string', 'control': 'textedit'},
 
            {'name':'select_x', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'select_y', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'enabled_source', 'default':"", 'type':'source', 'control': 'textedit'},
        ]};

    static html()
    {
        return "<button type='button' class=''></button>";
    }



    requestData(data_set)
    {
        this.addSource(data_set, this.parameters.parameter);
        if(this.parameters.file_names_source)
            this.addSource(data_set, this.parameters.file_names_source);
        if(this.parameters.enabled_source)
            this.addSource(data_set, this.parameters.enabled_source);
    }

    getSelectX()
    {
        if(this.parameters.select_x !== undefined && this.parameters.select_x !== "")
            return Number(this.parameters.select_x);
        return 0;
    }

    getSelectY()
    {
        if(this.parameters.select_y !== undefined && this.parameters.select_y !== "")
            return Math.trunc(Number(this.parameters.select_y));
        return 0;
    }

    getButtonBackground()
    {
        if(this.parameters.background_color !== undefined && this.parameters.background_color !== "")
            return this.parameters.background_color;
        return this.parameters.background ?? "";
    }

    usesLegacyButtonBackground()
    {
        return (
            (this.parameters.background_color === undefined || this.parameters.background_color === "") &&
            this.parameters.background !== undefined &&
            this.parameters.background !== ""
        );
    }

    setSelected(selected)
    {
        this.firstChild.classList.toggle("button-selected", !!selected);
    }

    isSelected()
    {
        return this.firstChild.classList.contains("button-selected");
    }

    setPressed(pressed)
    {
        this.firstChild.classList.toggle("button-pressed", !!pressed);
    }

    isPressed()
    {
        return this.firstChild.classList.contains("button-pressed");
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
            thisbutton.parentElement.setPressed(true);
            if(p.parameter)
                this.parentElement.send_control_change(p.parameter, p.value, selectX, selectY);
            if(p.command)
                this.parentElement.send_command(p.command, p.value, selectX, selectY);
        }

        else if(p.type=="toggle")
        {
            if(!thisbutton.parentElement.isSelected())
            {
                thisbutton.parentElement.setSelected(true);
                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.value, selectX, selectY);
                if(p.command)
                    this.parentElement.send_command(p.command, p.value, selectX, selectY);

            }
            else
            {
                thisbutton.parentElement.setSelected(false);
                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.release_value, selectX, selectY);
                if(p.release_command)
                    this.parentElement.send_command(p.release_command, p.release_value, selectX, selectY);

            }
        }


            else if(p.type=="radio")
            {
                let buttons = document.getElementsByTagName("webui-widget-button");
                for(let b of buttons)
                {
                    if(b.parameters.radio_group == p.radio_group && b.firstChild.classList.contains("button-selected"))
                    {
                        b.setSelected(false);
                        let q = b.parameters;
                        if(q.parameter)
                            this.parentElement.send_control_change(q.parameter, q.release_value, b.getSelectX(), b.getSelectY());
                    }
                }
                thisbutton.parentElement.setSelected(true);

                if(p.parameter)
                    this.parentElement.send_control_change(p.parameter, p.value, selectX, selectY);
                if(p.command)
                    this.parentElement.send_command(p.command, p.value, selectX, selectY);
            }

            else if(p.type=="multi")
            {
                thisbutton.parentElement.setPressed(true);
                let buttons = document.getElementsByTagName("webui-widget-button");
                for(let b of buttons)
                {
                    if(b.parameters.multi_group == p.multi_group)
                        if(b.firstElementChild!=this)
                        {
                            b.firstElementChild.dispatchEvent(new Event('mousedown'));
                        }
                }
            }
            else if(p.type=="input")
            {
                if(main.edit_mode)
                    return;

                thisbutton.parentElement.setPressed(true);
            }


            else if(p.type=="open")
            {
                if(main.edit_mode)
                    return; // TEMPORARY

                thisbutton.parentElement.setPressed(true);
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
            this.parentElement.setPressed(false);
            if(p.parameter)
            {
                this.parentElement.send_control_change(p.parameter, p.release_value, selectX, selectY);
            }
            if(p.release_command)
                this.parentElement.send_command(p.release_command, p.release_value, selectX, selectY);
        }

        else if(p.type=="toggle")
        {

        }

        else if(p.type=="radio")
        {

        }

        else if(p.type=="multi")
        {
            this.parentElement.setPressed(false);
        }

        else if(p.type=="input")
        {
            if(!this.parentElement.isPressed())
                return;
            this.parentElement.setPressed(false);
            let text = prompt(p.title);
            if(text)
                this.parentElement.send_command(p.command, text, selectX, selectY);
        }

        else if(p.type=="open")
        {
            if(!this.parentElement.isPressed())
                return;
            this.parentElement.setPressed(false);
            let thisbutton = this;
            let callback = function (selected_item)
            {
                thisbutton.parentElement.send_command(p.command, selected_item, selectX, selectY);
            }

            if(this.file_names_source)
                dialog.showListSelectDialog(this.file_names_source, callback, p.title);
            else
                dialog.showListSelectDialog("", callback, p.title);
        }
    }

    init()
    {
        super.init();
        this.firstChild.addEventListener("mousedown", this.button_down, true);
        this.firstChild.addEventListener("mouseup", this.button_up, true);
        this.firstChild.addEventListener("mouseleave", () => this.setPressed(false), true);
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

        if(this.parameters.text_color)
            this.firstChild.style.color = this.parameters.text_color;

        this.firstChild.title = this.parameters.tooltip || "";

        const buttonBackground = this.getButtonBackground();
        this.firstChild.style.background = buttonBackground || "";
        if(this.usesLegacyButtonBackground())
            this.parentElement.style.background = "";

        if(this.parameters.file_names_source)
            this.firstChild.file_names_source = this.getSource("file_names_source");
        if(this.parameters.icon)
        {
            const iconClass = String(this.parameters.icon).endsWith("record.png") ? "button-icon button-icon-preserve-color" : "button-icon";
            const icon = document.createElement("img");
            icon.src = this.parameters.icon;
            icon.className = iconClass;
            this.firstChild.replaceChildren(icon);
        }
        else
            this.firstChild.innerText = this.parameters.label;

        try
        {
            if(this.parameters.enabled_source)
            {
                const enabled_source = this.getSource('enabled_source');
                const enableValue = Array.isArray(enabled_source) ? (Array.isArray(enabled_source[0]) ? enabled_source[0][0] : enabled_source[0]) : enabled_source;
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

                        this.setSelected(true)
                    else
                        this.setSelected(false)
                }

        }
        catch(err)
        {}
    }

}



webui_widgets.add('webui-widget-button', WebUIWidgetButton);
