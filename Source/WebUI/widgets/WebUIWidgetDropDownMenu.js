class WebUIWidgetDropDownMenu extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DROP DOWN MENU", 'control':'header'},
            {'name':'title', 'default':"Menu", 'type':'string', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'enabled_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'select_x', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'select_y', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'options_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'value_type', 'default':"number", 'type':'string', 'control': 'menu', 'options': "number,string"},
            {'name':'options', 'default':"X,Y,Z", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'label_width', 'default':50, 'type':'int', 'control': 'textedit'},
        ]};

    static html()
    {
         return "<label></label><select></select>";
    }

    requestData(data_set)
    {
        this.addSource(data_set, this.parameters.parameter);
        if(this.parameters.enabled_source)
            this.addSource(data_set, this.parameters.enabled_source);
        if(this.parameters.options_source)
            this.addSource(data_set, this.parameters.options_source);
    }

    isEnabled()
    {
        if(!this.parameters.enabled_source)
            return true;

        const enabled_source = this.getSource('enabled_source', 1);
        const enableValue = Array.isArray(enabled_source) ? (Array.isArray(enabled_source[0]) ? enabled_source[0][0] : enabled_source[0]) : enabled_source;
        return Number(enableValue) !== 0;
    }

    syncEnabledState()
    {
        const selector = this.querySelector("select");
        const enabled = this.isEnabled();
        const interactive = enabled && !main.edit_mode;
        this.classList.toggle("widget-control-disabled", !interactive);
        if(selector)
        {
            selector.disabled = !interactive;
            selector.style.pointerEvents = main.edit_mode ? "none" : "";
        }
    }

    option_selected(index, value, text)
    {
        if(!this.parameters.parameter)
            return;

        const x = this.getSelectX();
        const y = this.getSelectY();
        const selectedValue = this.parameters.value_type=='string' ? text : value;
        if(y === "")
            this.send_control_change(this.parameters.parameter, selectedValue, x);
        else
            this.send_control_change(this.parameters.parameter, selectedValue, x, Number.isFinite(Number(y)) ? Math.max(0, Math.trunc(Number(y))) : 0);
    }

    getSelectX()
    {
        const value = Number(this.parameters.select_x);
        return Number.isFinite(value) ? Math.max(0, Math.trunc(value)) : 0;
    }

    getSelectY()
    {
        if(this.parameters.select_y === undefined || this.parameters.select_y === null)
            return "";
        return this.parameters.select_y;
    }


    changeOptions(options)
    {
        let selector = this.querySelector("select")
        const optionString = Array.isArray(options) ? options.flat(Infinity).join(",") : String(options ?? "");
        if(this._optionsKey === optionString)
            return;
        this._optionsKey = optionString;
        while(selector.childElementCount > 0)
            selector.removeChild(selector.children[0]);

        let l = optionString === "" ? [] : optionString.split(',');
        let ix = 0;
        for(let e of l)
        {
            let node = document.createElement("option");
            let textnode = document.createTextNode(e.trim());
            node.appendChild(textnode);
            node.setAttribute("value", ix);
            selector.appendChild(node);
            ix++;
        }
    }
    
    updateAll() {
        super.updateAll();
    
        let selector = this.querySelector("select");
        selector.onchange = (e) => {
            if(main.edit_mode)
                return;
            e.preventDefault();
            e.stopPropagation();
            const selectedText = e.target.selectedOptions?.[0]?.innerText ?? "";
            this.option_selected(this.getSelectX(), e.target.value, selectedText);
        };

        selector.onmousedown = (e) => {
            if(!main.edit_mode)
                e.stopPropagation();
        };

        this.changeOptions(this.parameters.options);
        const label = this.querySelector("label");
        label.innerText = this.parameters.label;
        const configuredLabelWidth = Number(this.parameters.label_width);
        const labelWidth = this.parameters.label && Number.isFinite(configuredLabelWidth) ? Math.max(0, configuredLabelWidth) : 0;
        label.style.display = labelWidth ? "inline-block" : "none";
        label.style.width = `${labelWidth}px`;
        selector.style.width = labelWidth ? `calc(100% - ${labelWidth}px)` : "100%";
        this.syncEnabledState();
    }
    update()
    {
        this.syncEnabledState();
        if(this.parameters.options_source)
            this.changeOptions(this.getSource('options_source'));

        const d = this.getSource('parameter');
        const selectElement = this.querySelector("select");
        if(d === undefined || d === null)
        {
            selectElement.selectedIndex = -1;
            return;
        }

        const selectX = this.getSelectX();
        const selectY = this.getSelectY();
        let value = d;
        if(Array.isArray(d))
        {
            if(selectY !== "" && Array.isArray(d[0]))
            {
                const configuredY = Number(selectY);
                const y = Number.isFinite(configuredY) ? Math.max(0, Math.trunc(configuredY)) : 0;
                value = d[y]?.[selectX];
            }
            else
                value = Array.isArray(d[0]) ? d[0][selectX] : d[selectX];
        }

        if(this.parameters.value_type=='number')
        {
                selectElement.value = value ?? selectElement.value;
                return;
        }

        const stringValue = String(value ?? "");
        for (let i = 0; i < selectElement.options.length; i++)
        {
            if (selectElement.options[i].text === stringValue)
            {
                selectElement.selectedIndex = i;
                return;
            }
        }
        selectElement.selectedIndex = -1;
    }
};


webui_widgets.add('webui-widget-drop-down-menu', WebUIWidgetDropDownMenu);
