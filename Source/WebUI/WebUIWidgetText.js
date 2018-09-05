class WebUIWidgetText extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            {'name':'title', 'default':"Default Title", 'type':'string', 'control': 'textedit'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'text', 'default':"", 'type':'string', 'control': 'textedit'},
            
            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<div> </div>";
    }

    requestData(data_set)
    {
        if(!this.parameters.text)
            data_set.add(this.parameters.module+"."+this.parameters.parameter);
    }
/*
    text_edited(index, value)
    {
        if(this.parameters.module && this.parameters.parameter)
            this.get("/control/"+this.parameters.module+"/"+this.parameters.parameter+"/"+index+"/0/"+value);
    }
*/
    init()
    {
        this.text = this.parameters.text;
        this.innerText = this.text;
    }
    
    update(d)
    {
         try {
            if(this.parameters.text)
            {
                this.text = this.parameters.text;
                this.innerText = this.text;
                return;
            }
         
            let m = this.parameters.module;
            let s = this.parameters.parameter;
            this.data = d[m][s];

            if(this.data)
            {
                this.text = this.data;
                this.innerText = this.text;
            }
            else
            {
                this.text = this.parameters.text;
                this.data = this.text;
                this.innerText = this.text;
            }
        }
        catch(err)
        {
        
        }
    }
};



webui_widgets.add('webui-widget-text', WebUIWidgetText);

