class WebUIWidgetText extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "TEXT", 'control':'header'},
            {'name':'title', 'default':"Default Title", 'type':'string', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'text', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'prefix', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'postfix', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'separator', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'strings', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'select_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name': "FRAME", 'control':'header'},
            {'name':'background', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame_color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame_width', 'default':"1", 'type':'int', 'control': 'textedit'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<div> </div>";
    }

    beginInlineTextEdit(evt)
    {
        if(!main.edit_mode || this.inline_text_edit)
            return;
        if(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
        }

        const content = this.firstChild;
        if(!content)
            return;

        this.inline_text_edit = {
            originalText: content.textContent ?? ""
        };

        content.contentEditable = "true";
        content.setAttribute("contenteditable", "true");
        content.spellcheck = false;
        content.classList.add("inline-widget-text-edit");
        content.focus({preventScroll:true});
        document.getSelection()?.selectAllChildren(content);
    }

    finishInlineTextEdit(commit)
    {
        if(!this.inline_text_edit)
            return;

        const content = this.firstChild;
        const session = this.inline_text_edit;
        this.inline_text_edit = null;
        if(!content)
            return;

        const nextText = (content.textContent ?? "").replace(/\n/g, "").trim();
        content.contentEditable = "false";
        content.removeAttribute("contenteditable");
        content.classList.remove("inline-widget-text-edit");

        if(commit)
        {
            const fullName = this.parentElement && this.parentElement.dataset ? this.parentElement.dataset.name : "";
            if(fullName && network.dict[fullName])
            {
                network.dict[fullName].text = nextText;
                this.parameters.text = nextText;
                this.text = nextText;
                if(inspector && typeof inspector.showInspectorForSelection === "function")
                    inspector.showInspectorForSelection();
                network.tainted = true;
            }
            this.setDisplayedText(nextText);
        }
        else
            this.setDisplayedText(session.originalText);
    }

    setDisplayedText(value)
    {
        if(this.firstChild)
            this.firstChild.textContent = value ?? "";
        else
            this.textContent = value ?? "";
    }

    updateFrame()
    {
        let fcolors = String(this.parameters.frame_color ?? "").split(',').map((c) => c.trim()).filter((c) => c !== "");
        if(fcolors.length > 0)
        {
            this.parentElement.style.borderTopColor = fcolors[0];
            this.parentElement.style.borderRightColor = fcolors[1 % fcolors.length];
            this.parentElement.style.borderBottomColor = fcolors[2 % fcolors.length];
            this.parentElement.style.borderLeftColor = fcolors[3 % fcolors.length];
        }
        else
        {
            this.parentElement.style.borderTopColor = "";
            this.parentElement.style.borderRightColor = "";
            this.parentElement.style.borderBottomColor = "";
            this.parentElement.style.borderLeftColor = "";
        }

        let fw = this.parameters.frame_width;
        this.parentElement.style.borderWidth = fw ? fw + "px" : "";
        this.parentElement.style.background = this.parameters.background;

        super.updateFrame();
    }
/*
    requestData(data_set)
    {
        if(!this.parameters.text)
            data_set.add(this.parameters.parameter);
    }
*/
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
        this.setDisplayedText(this.text);
        const content = this.firstChild;
        if(content)
        {
            content.addEventListener("dblclick", this.beginInlineTextEdit.bind(this), false);
            content.addEventListener("keydown", (evt) =>
            {
                if(!this.inline_text_edit)
                    return;
                if(evt.key === "Enter")
                {
                    evt.preventDefault();
                    this.finishInlineTextEdit(true);
                    content.blur();
                    return;
                }
                if(evt.key === "Escape")
                {
                    evt.preventDefault();
                    this.finishInlineTextEdit(false);
                    content.blur();
                }
            }, true);
            content.addEventListener("blur", () =>
            {
                if(this.inline_text_edit)
                    this.finishInlineTextEdit(true);
            }, true);
        }
    }
    
    update()
    {
         try {
            if(this.inline_text_edit)
                return;
            if(this.parameters.text)
            {
                this.text = this.parameters.text;
                this.setDisplayedText(this.text);
                return;
            }
         
            else if(this.text = this.getSource('parameter'))
            {
                this.setDisplayedText(this.text);
            }

            this.data = this.getSource('select_source')
            if(this.data && this.parameters.strings)
            {
                let sep = this.parameters.separator || "";
                let ss = String(this.parameters.strings ?? "").split(",")
                let s = [];
                if (!Array.isArray(this.data))
                    return;
                if (!Array.isArray(this.data[0]))
                    this.data = [this.data];
                if (!Array.isArray(this.data[0]))
                    return;
                for(let i=0; i<this.data[0].length; i++)
                    if(this.data[0][i] > 0 && typeof ss[i] !== "undefined")
                        s.push(ss[i].trim());
                this.setDisplayedText((this.parameters.prefix || "") + s.join(sep) + (this.parameters.postfix || ""));

            }
        }
        catch(err)
        {
        
        }
    }
};

webui_widgets.add('webui-widget-text', WebUIWidgetText);
