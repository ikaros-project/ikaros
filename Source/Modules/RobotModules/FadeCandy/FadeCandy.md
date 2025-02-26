<?xml version="1.0"?>

<class name="FadeCandy" description="Allows control of NeoPixels hardware through FadeCandy server">

	<description type="text">
        Ikaros module for FadeCandy that allows Ikaros to control NeoPixels through a FadeCandy (https://github.com/scanlime fadecandy). 
		The module starts the fadecandy server using the name set by command. The binary is assumed to be placed next to the module. 
		If command is set to "" (empty string), the server will not be started.

        The different channels of NeoPixels are defined by a channel-elemnt that sets the name, channel numer and size. 
		Three inputs for the red, green and blue colours values are created for each channel. 
		If the channel is named X, the input will be named X_RED, X_GREEN and X_BLUE.

        The data sent uses the default format for FadeCandy where the data for each channel 
		sent to the server is assumed to start at position 4+64*channel*3 within a single package.
	</description>

	<example description="A simple example">
		<module
			class="FadeCandy"
			name="FadeCandy"
		>
            <channel name="LED_RING_1" channel="0" size="12" />
            <channel name="PIXEL_STRIP" channel="1" size="8" />
        </module>
	</example>


	<parameter name="command" type="string" description="Name of the fadecandy server" default="fcserver-osx" />
	<parameter name="start_server" type="bool" description="Should the module start the fadecandy server" default="true" />


	<input name="*" description="The channels can have arbitrary names" />

	<link class="FadeCandy" />

	<author>
		<name>Christian Balkenius</name>
		<email>christian.balkenius@lucs.lu.se</email>
		<affiliation>Lund University Cognitive Science</affiliation>
		<homepage>http://www.lucs.lu.se/Christian.Balkenius</homepage>
	</author>

   <files>
   		<file>FadeCandy.h</file>
   		<file>FadeCandy.cc</file>
   		<file>FadeCandy.ikc</file>
   </files>

    <limitation>
        Only works with NeoPixel-arrays with a maximum of 64 pixels each.
    </limitation>

</class>