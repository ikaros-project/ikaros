class WebUIWidgetDropDownMenu extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            {'name':'title', 'default':"Sliders", 'type':'string', 'control': 'textedit'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'index', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'list_module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'list_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'list', 'default':"", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'labelWidth', 'default':50, 'type':'int', 'control': 'textedit'},

/*

            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'step', 'default':0.01, 'type':'float', 'control': 'textedit'},
            {'name':'show_values', 'default':false, 'type':'bool', 'control': 'checkbox'},
*/
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

    requestData(data_set)
    {
        data_set.add(this.parameters.module+"."+this.parameters.parameter);
        if(this.parameters.list_module && this.parameters.list_parameter)
            data_set.add(this.parameters.list_module+"."+this.parameters.list_parameter);
    }

    option_selected(index, value)
    {
        if(this.parameters.module && this.parameters.parameter)
            this.get("/control/"+this.parameters.module+"/"+this.parameters.parameter+"/"+index+"/0/"+value);
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

    update(d)
    {
         try {
            let m = this.parameters.module;
            let s = this.parameters.parameter;
            this.data = d[m][s];

            if(this.data)
            {
                let size_y = this.data.length;
                let size_x = this.data[0].length;
 
                let selector = this.querySelector("select");

                if(this.parameters.list_module && this.parameters.list_parameter)
                    this.changeOptions(d[this.parameters.list_module][this.parameters.list_parameter])
 
                selector.value = this.data[this.parameters.index][0];
            }
        }
        catch(err)
        {
        
        }
    }
};


webui_widgets.add('webui-widget-drop-down-menu', WebUIWidgetDropDownMenu);

