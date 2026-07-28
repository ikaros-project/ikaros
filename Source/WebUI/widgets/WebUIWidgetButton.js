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
        if(main.edit_mode)
            return;
        evt.stopPropagation();
        let thisbutton = this;
        let p = this.parentElement.parameters;
        const selectX = this.parentElement.getSelectX();
        const selectY = this.parentElement.getSelectY(0);

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
                            this.parentElement.send_control_change(q.parameter, q.release_value, b.getSelectX(), b.getSelectY(0));
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
                    if(b.parameters.multi_group == p.multi_group && b.parameters.type !== "multi")
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
        evt.stopPropagation();
        if(main.edit_mode)
            return;
        let p = this.parentElement.parameters;
        const selectX = this.parentElement.getSelectX();
        const selectY = this.parentElement.getSelectY(0);

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
        this.firstChild.addEventListener("mouseleave", (evt) =>
        {
            if(this.isPressed())
                this.button_up.call(this.firstChild, evt);
        }, true);
        this.firstChild.addEventListener('click', e => {
            if(main.edit_mode)
                return; 
            e.stopPropagation();
            }, true);
    }

    update(d)
    {
        this.parameters.select_x = this.getSelectX();
        this.parameters.select_y = this.getSelectY(0);

        this.firstChild.style.color = this.parameters.text_color || "";

        this.firstChild.title = this.parameters.tooltip || "";

        const buttonBackground = this.getButtonBackground();
        this.firstChild.style.background = buttonBackground || "";
        if(this.usesLegacyButtonBackground())
            this.parentElement.style.background = "";

        this.firstChild.file_names_source = this.parameters.file_names_source ? this.getSource("file_names_source") : "";
        if(this.parameters.icon)
        {
            const iconClass = String(this.parameters.icon).endsWith("record.png") ? "button-icon button-icon-preserve-color" : "button-icon";
            let icon = this.firstChild.firstElementChild;
            if(!icon || icon.tagName !== "IMG")
            {
                icon = document.createElement("img");
                this.firstChild.replaceChildren(icon);
            }
            if(icon.getAttribute("src") !== String(this.parameters.icon))
                icon.src = this.parameters.icon;
            icon.className = iconClass;
        }
        else
            this.firstChild.innerText = this.parameters.label;

        this.syncControlEnabledState([this.firstChild]);

        if(this.parameters.parameter)
        {
            const sourceValue = this.getSelectedSourceValue('parameter');
            this.setSelected(sourceValue == this.parameters.value);
        }
    }

}



webui_widgets.add('webui-widget-button', WebUIWidgetButton);
