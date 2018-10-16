class WebUIWidgetTable extends WebUIWidget
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            {'name':'title', 'default':"Default Title", 'type':'string', 'control': 'textedit'},
 //           {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            
            {'name': "STYLE", 'control':'header'},
            {'name':'decimals', 'default': 4, 'type':'int', 'control': 'textedit'},
            {'name':'colorize', 'default':true, 'type':'bool', 'control': 'checkbox'},

            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<table> </table>";
    }

    init()
    {
        this.div = this.querySelector('div');
        this.table = this.querySelector('table');
        this.rows = 0;
        this.cols = 0;
    }

    reshapeTable(r, c)
    {
        this.rows = r;
        this.cols = c;
        
        while(this.table.rows.length)
            this.deleteRow(-1);

        for(let j=0; j<r; j++)
        {
            let new_row = this.table.insertRow(0);
            for(let i=0; i<c; i++)
            {
                let cell = new_row.insertCell(i);
                cell.innerHTML = "x";
            }
        }
    }

    update()
    {
        if(this.data = this.getSource('source'))
        {
            let size_y = this.data.length;
            let size_x = this.data[0].length;

            if(this.rows != size_y || this.cols != size_x)
                this.reshapeTable(size_y, size_x);
            
            if(this.parameters.colorize)
                for(let j=0; j<size_y; j++)
                    for(let i=0; i<size_x; i++)
                    {
                        this.table.rows[j].cells[i].innerHTML = this.data[j][i].toFixed(this.parameters.decimals);
                        this.table.rows[j].cells[i].style.color = this.getColor(i, this.data[j][i]);
                    }
            else
               for(let j=0; j<size_y; j++)
                    for(let i=0; i<size_x; i++)
                    {
                        this.table.rows[j].cells[i].innerHTML = this.data[j][i].toFixed(this.parameters.decimals);;
                        this.table.rows[j].cells[i].style.color = this.getColor(i);
                    }

        }
    }
};



webui_widgets.add('webui-widget-table', WebUIWidgetTable);

