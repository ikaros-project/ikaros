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

    requestData(data_set)   // TODO: Remove - is automatic now
    {
        data_set.add(this.parameters.parameter);
        if(this.parameters.list_parameter)
            data_set.add(this.parameters.list_parameter);
    }

    option_selected(index, value)
    {
        if(this.parameters.parameter)
            this.get("/control/"+this.parameters.parameter+"/"+index+"/0/"+value);
    }

    changeOptions(options)
    {
        let selector = this.querySelector("select")
        while(selector.childElementCount > 0)
            selector.removeChild(selector.children[0]);

        let l = options.split(',');
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
    
    updateAll()
    {
        super.updateAll();
 
        let selector = this.querySelector("select")
        selector.onchange = function (e) {
            this.parentElement.option_selected(this.parentElement.parameters.index, e.target.value);
        };

         this.changeOptions(this.parameters.list);
        
         this.querySelector("label").innerText = this.parameters.label;
    }

    update()
    {
         try {
            let d = this.getSource('parameter');
            if(d)
            {
                let size_y = d.length;
                let size_x = d[0].length;
 
                let selector = this.querySelector("select");

                if(this.parameters.list_parameter)
                {
                    let l = this.getSource('list_parameter');
                    this.changeOptions(l)
                }
 
                selector.value = d[this.parameters.index][0];
            }
        }
        catch(err)
        {
        
        }
    }
};


webui_widgets.add('webui-widget-drop-down-menu', WebUIWidgetDropDownMenu);

