class WebUIWidgetControl extends WebUIWidget
{
    // TODO: these should be changed to go through command object to allow faster update

    send_control_change(parameter, value=0, index_x=0, index_y=0)
    {
        if(this.groupName)
            this.get("/control/"+this.groupName+"."+parameter+"/"+index_x+"/"+index_y+"/"+value);
        else
            this.get("/control/"+parameter+"/"+index_x+"/"+index_y+"/"+value);
    }

    send_command(command, value=0, index_x=0, index_y=0)
    {
        if(this.groupName)
            this.get("/command/"+this.groupName+"."+command+"/"+index_x+"/"+index_y+"/"+value);
        else
            this.get("/command/"+command+"/"+index_x+"/"+index_y+"/"+value);
    }

};

webui_widgets.add('webui-widget-control', WebUIWidgetControl); // ???


