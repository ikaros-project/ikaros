class WebUIWidgetSwitch extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            {'name':'title', 'default':"Switch Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"Press", 'type':'string', 'control': 'textedit'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'single_trig', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'value', 'default':1, 'type':'int', 'control': 'textedit'},
            {'name':'xindex', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'yindex', 'default':0, 'type':'int', 'control': 'textedit'},

            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<div><input type='checkbox'/><label></label></div>";
    }

    requestData(data_set)
    {
        data_set.add(this.parameters.module+"."+this.parameters.parameter);
    }

    button_up()
    {
        let s = this.querySelector("input");
        let p = this.parameters;
        if(p.module && p.parameter)
        {
            if(s.checked)
                this.get("/control/"+p.module+"/"+p.parameter+"/"+p.xindex+"/"+p.yindex+"/0");
            else
                this.get("/control/"+p.module+"/"+p.parameter+"/"+p.xindex+"/"+p.yindex+"/1");
        }
    }

    init()
    {
        super.init();
        this.querySelector('input').onmouseup = function (){
            this.parentElement.parentElement.button_up();
        }
    }

    update(d)
    {
         try {
            this.querySelector('label').innerText = this.parameters.label;

            let m = this.parameters.module;
            let s = this.parameters.parameter;
            this.data = d[m][s];

            if(this.data)
            {
                let size_y = this.data.length;
                let size_x = this.data[0].length;
                let s = this.querySelector("input");
                s.checked = (this.data[this.parameters.yindex][this.parameters.xindex] > 0);
            }
        }
        catch(err)
        {

        }
    }
};


webui_widgets.add('webui-widget-switch', WebUIWidgetSwitch);
