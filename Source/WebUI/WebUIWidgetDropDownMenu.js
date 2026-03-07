class WebUIWidgetDropDownMenu extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DROP DOWN MENU", 'control':'header'},
            {'name':'title', 'default':"Menu", 'type':'string', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'index', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'list_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter_type', 'default':"number", 'type':'string', 'control': 'menu', 'options': "number,string"},
            {'name':'list', 'default':"X,Y,Z", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'labelWidth', 'default':50, 'type':'int', 'control': 'textedit'},

            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
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
        if(this.parameters.parameter)
            if(this.parameters.parameter_type=='string')
                this.send_control_change(this.parameters.parameter, text);
            else
                this.send_control_change(this.parameters.parameter, value);
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
    selector.addEventListener('change', (e) => {
        e.preventDefault();  // Prevent default action
        e.stopPropagation(); // Stop event bubbling
        const selectedText = e.target.selectedOptions?.[0]?.innerText ?? "";
        this.option_selected(this.parameters.index, e.target.value, selectedText);
    }, true);  // Use capture phase

    selector.addEventListener('mousedown', (e) => {
        //e.preventDefault();  // Prevent default action
        e.stopPropagation(); // Stop event bubbling
    }, true);  // Use capture phase

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
                const index = Number(this.parameters.index) || 0;
                let value = d;
                if(Array.isArray(d))
                    value = Array.isArray(d[0]) ? d[0][index] : d[index];
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
