class WebUIWidgetSliderHorizontal extends WebUIWidgetSlider
{
    static template()
    {
        return super.template("SLIDER HORIZONTAL");
    }

    static html()
    {
        return '<div class="hranger"></div>';
    }
}

webui_widgets.add("webui-widget-slider-horizontal", WebUIWidgetSliderHorizontal);
