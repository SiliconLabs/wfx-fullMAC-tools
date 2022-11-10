# Restrictions of EFR32™ SoC Wireless Starter Kit

This note explains why EFR32xG21/xG24 Wireless Starter Kits cannot directly connect to the WF200 module for supporting full features of the Multiprotocol example.

## Expected Overall Layout

The picture below presents the expected layout of EFR32 Wireless Starter Kit with WF200 module which is connected through an on-board **expansion header** (plug-and-play mode).

<p align="center">
<img src="Kit_Layout.png" width="100%" title="Kit Layout" alt="mg12_create_project"/>
</p>

<br>

> **Note:**  References
>   *   [**SLWSTK6006A: EFR32xG21 Wireless Gecko Starter Kit**](https://www.silabs.com/development-tools/wireless/efr32xg21-wireless-starter-kit?tab=techdocs)
>   *   [**UG526: EFR32xG24 2.4 GHz 20 dBm Radio Board User's Guide**](https://www.silabs.com/documents/public/user-guides/ug526-brd4187c-user-guide.pdf)
>   *   [**UG379: WF200 Wi-Fi Expansion Kit User's Guide**](https://www.silabs.com/documents/public/user-guides/ug379_slexpwfx200-users-guide.pdf)


<br>

## EFR32xG21 Wireless Gecko Starter Kit Limitation

Considering the [**EFR32xG21 2.4 GHz 20 dBm Wireless Starter Kit User's Guide**](https://www.silabs.com/documents/public/user-guides/ug385-brd4180a-user-guide.pdf) (others have similar specs), _**EFR32xG21 pins are shared with many on-board modules and many pads are not connected (NC)**_ as can be seen in the following image of breakout pad ping mapping.
<p align="center">
<img src="xg21_breakout_pad.png" width="70%" title="xG21 breakout pad" alt="xG21 breakout pad"/>
</p>

According to **Pin Mapping** section (page 3) in [**BRD4180A Rev. A00 Schematic PDF file**](https://www.silabs.com/documents/public/schematic-files/BRD4180A-A00-schematic.pdf), the development board _**doesn't support the plug-and-play mode and can't support full features**_ of the _**Multiprotocol example**_ because:

1. EXP_HEADER3, EXP_HEADER4, EXP_HEADER15 are routed to EFR32_PC00 and  EXP_HEADER5, EXP_HEADER8, EXP_HEADER16 are routed to EFR32_PC02. This is the main reason causing EFR32xG21 doesn't have plug-and-play mode.

2. Push buttons must be disabled because UFI_BUTTON0 and UFI_BUTTON1 are also connected to EXP_HEADER7, EXP_HEADER9 respectively. See light yellow boxes in the capture below:
   
    <p align="center">
    <img src="xg21_pin_mapping.png" width="100%" title="xG21 pin mapping" alt="xG21 pin mapping"/>
    </p>

3. Either LCD screen or VCOM is disabled because their enable pins share the same GPIO EFR32_PD4 pin
    <p align="center">
    <img src="VCOM_LCD_disabled.png" width="100%" title="VCOM or LCD disabled" alt="VCOM or LCD disabled"/>
    </p>

<br>
<br>

## EFR32xG24 Wireless Gecko Starter Kit Limitation

The **Connectors** section in [**EFR32xG24 2.4 GHz 20 dBm Radio Board User's Guide**](https://www.silabs.com/documents/public/user-guides/ug526-brd4187c-user-guide.pdf) (others have similar specs) also shows _**EFR32xG24 pins are shared with many on-board modules**_ as the following breakout pad pin mapping image.
<p align="center">
<img src="xg24_breakout_pad.png" width="50%" title="xG24 breakout pad" alt="xG24 breakout pad"/>
</p>

According to **Pin Mapping** section (page 3) in [**BRD4187C Rev. A01 Schematic PDF file**](https://www.silabs.com/documents/public/schematic-files/BRD4187C-A01-schematic.pdf), the development board _**can't support full features in the plug-and-play mode**_ because:

1. EXP_HEADER12 and EXP_HEADER14 are connected to VCOM_TX, VCOM_RX, which causes VCOM component must be disabled.

2. LCD screen component is unavailable because EXP_HEADER4 and EXP_HEADER8 are also routed to DISP_SI, DISP_SCLK pins. See light yellow boxes in the capture below:
   
    <p align="center">
    <img src="xg24_pin_mapping.png" width="100%" title="xG24 pin mapping" alt="xG24 pin mapping"/>
    </p>

<br>

## How to retrieve the related documents of a specific board?

Related documents can be downloaded from Simplicity Studio 5. Open **Launcher** tab, press **Add product (Plus sign)** and then enter your development board (e.g., EFR32xG21 radio board with BRD4180A). 

<p align="center">
<img src="launcher_board.png" width="70%" title="Launcher Board" alt="Launcher Board"/>
</p>

Under **Overview** tab, click on **View Documents** drop box and select desire document

<p align="center">
<img src="view_documents.png" width="70%" title="View Documents" alt="View Documents"/>
</p>