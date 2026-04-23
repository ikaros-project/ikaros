class WebUIWidgetDropDownMenu extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DROP DOWN MENU", 'control':'header'},
            {'name':'title', 'default':"Menu", 'type':'string', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'select_x', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'select_y', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'list_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter_type', 'default':"number", 'type':'string', 'control': 'menu', 'options': "number,string"},
            {'name':'list', 'default':"X,Y,Z", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'labelWidth', 'default':50, 'type':'int', 'control': 'textedit'},
        ]};

    static html()
    {
         return "<label></label><select></select>";
    }

    /*
    requestData(data_set)   // TODO: Remove - is automatic now
    {
        data_set.add(this.parameters.parameter);
        if(this.parameters.list_parameter)
            data_set.add(this.parameters.list_parameter);
    }
*/

    option_selected(index, value, text)
    {
        if(!this.parameters.parameter)
            return;

        const x = this.getSelectX();
        const y = this.getSelectY();
        const selectedValue = this.parameters.parameter_type=='string' ? text : value;
        if(y === "")
            this.send_control_change(this.parameters.parameter, selectedValue, x);
        else
            this.send_control_change(this.parameters.parameter, selectedValue, x, Math.trunc(Number(y)));
    }

    getSelectX()
    {
        if(this.parameters.select_x !== undefined && this.parameters.select_x !== "")
            return Number(this.parameters.select_x);
        return Number(this.parameters.index ?? 0);
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
        while(selector.childElementCount > 0)
            selector.removeChild(selector.children[0]);

        const optionString = Array.isArray(options) ? options.join(",") : String(options ?? "");
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
            e.preventDefault();
            e.stopPropagation();
            const selectedText = e.target.selectedOptions?.[0]?.innerText ?? "";
            this.option_selected(this.getSelectX(), e.target.value, selectedText);
        };

        selector.onmousedown = (e) => {
            e.stopPropagation();
        };

        this.changeOptions(this.parameters.list);
        this.querySelector("label").innerText = this.parameters.label;
    }
    update()
    {
         try
         {
            let d = this.getSource('parameter');
            if(!d)
                return;
            let selectElement = this.querySelector("select");
            if(this.parameters.list_parameter)
            {
                let l = this.getSource('list_parameter');
                this.changeOptions(l)
            }
                // FIXME: Check parameter type here
                // FIXME: Populate menu from parameter here or at init

                if(this.parameters.parameter_type=='number')
            {
                const selectX = this.getSelectX();
                const selectY = this.getSelectY();
                let value = d;
                if(Array.isArray(d))
                {
                    if(selectY !== "" && Array.isArray(d[0]))
                        value = d[Math.trunc(Number(selectY))]?.[selectX];
                    else
                        value = Array.isArray(d[0]) ? d[0][selectX] : d[selectX];
                }
                selectElement.value = value ?? selectElement.value;
                return;
            }

            const stringValue = String(Array.isArray(d) ? (Array.isArray(d[0]) ? d[0][0] : d[0]) : d);
            for (let i = 0; i < selectElement.options.length; i++)
            {
                if (selectElement.options[i].text === stringValue)
                {    
                    selectElement.selectedIndex = i;
                    return;
                }
            }

        }
        catch(err)
        {
        
        }
    }
};


webui_widgets.add('webui-widget-drop-down-menu', WebUIWidgetDropDownMenu);
