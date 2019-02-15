class WebUIWidgetSwitch extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "SWITCH", 'control':'header'},
            {'name':'title', 'default':"Switch Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"Press", 'type':'string', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},
//            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
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
        data_set.add(this.parameters.parameter);
    }

    button_up()
    {
        let s = this.querySelector("input");
        let p = this.parameters;
        if(p.parameter)
        {
            if(s.checked)
                this.get("/control/"+p.parameter+"/"+p.xindex+"/"+p.yindex+"/0");
            else
                this.get("/control/"+p.parameter+"/"+p.xindex+"/"+p.yindex+"/1");
        }
    }

    init()
    {
        super.init();
        this.querySelector('input').onmouseup = function (){
            this.parentElement.parentElement.button_up();
        }
    }

    update()
    {
         try {
            this.querySelector('label').innerText = this.parameters.label;

            let d = this.getSource("parameter");
            if(d)
            {
                let size_y = d.length;
                let size_x = d[0].length;
                let s = this.querySelector("input");
                s.checked = (d[this.parameters.yindex][this.parameters.xindex] > 0);
            }
        }
        catch(err)
        {

        }
    }
};


webui_widgets.add('webui-widget-switch', WebUIWidgetSwitch);
