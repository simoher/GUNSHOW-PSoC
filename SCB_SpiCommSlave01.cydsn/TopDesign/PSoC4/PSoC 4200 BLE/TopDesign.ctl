-- =============================================================================
-- The following directives assign pins to the locations specific for the
-- CY8CKIT-042-BLE kit.
-- =============================================================================

-- === SPIS (SCB mode) ===
attribute port_location of \SPIS:miso_s(0)\ : label is "PORT(0,1)";
attribute port_location of \SPIS:mosi_s(0)\ : label is "PORT(0,0)";
attribute port_location of \SPIS:sclk_s(0)\ : label is "PORT(0,3)";
attribute port_location of \SPIS:ss0_s(0)\  : label is "PORT(0,2)";

-- === SPIM (UDB based) ===
attribute port_location of miso_m(0) : label is "PORT(2,1)";
attribute port_location of mosi_m(0) : label is "PORT(2,2)";
attribute port_location of sclk_m(0) : label is "PORT(2,0)";
attribute port_location of ss_m(0)   : label is "PORT(2,3)";

-- === RGB LED ===
attribute port_location of LED_RED(0)   : label is "PORT(2,6)";
attribute port_location of LED_GREEN(0) : label is "PORT(3,6)";
attribute port_location of LED_BLUE(0)  : label is "PORT(3,7)";
