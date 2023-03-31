<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml" version="1.0">
  <xsl:output method="xml" indent="yes" encoding="UTF-8"/>
  <xsl:template match="/candb">
    <html>
      <head>
        <title>CAN DB</title>
        <!-- Add some very basic styling to the document, nothing too fancy -->
        <style type="text/css">body{line-height:1.6;font-size:16px;color:#444;padding:0 10px}h1,h2,h3{line-height:1.2}</style>
      </head>
      <body>
        <h1>CAN DB</h1>
        <!-- A table of all CAN messages in the database, these will be linked to a list of all CAN signals -->
        <table>
          <tr>
            <th>Message</th>
            <th>Id. (decimal)</th>
            <th>DLC</th>
            <th>Extended</th>
            <th>Signals</th>
          </tr>
          <xsl:apply-templates select="message">
            <xsl:sort select="name"/>
          </xsl:apply-templates>
        </table>
        <table>
          <tr>
            <th>Signal</th>
            <th>Start Bit</th>
            <th>Bit Length</th>
            <th>Endianess</th>
            <th>Scaling</th>
            <th>Offset</th>
            <th>Minimum</th>
            <th>Maximum</th>
            <th>Signed</th>
            <th>Units</th>
          </tr>
          <xsl:apply-templates select="message/signal">
            <xsl:sort select="name"/>
          </xsl:apply-templates>
        </table>
      </body>
    </html>
  </xsl:template>
  <!-- used to generate the message table -->
  <xsl:template match="message">
    <tr>
      <td> <xsl:value-of select="name"/> </td>
      <td> <xsl:value-of select="id"/> </td>
      <td> <xsl:value-of select="dlc"/> </td>
      <td> <xsl:value-of select="extended"/> </td>
      <td>
        <xsl:apply-templates select="signal" mode="signal-name">
          <xsl:sort select="name"/>
        </xsl:apply-templates>
      </td>
    </tr>
  </xsl:template>
  <!-- signal-name is used to create a reference in the message table -->
  <xsl:template match="signal" mode="signal-name">
    <tr>
      <td>
        <!-- select message name (from parent) and the signals name, to guarantee uniqueness -->
        <a href="#{../name}-{name}">
          <xsl:value-of select="name"/>
        </a>
      </td>
    </tr>
  </xsl:template>
  <!-- this is used to populate a table of all signals -->
  <xsl:template match="signal">
    <tr>
      <td> <a id="{../name}-{name}"> <xsl:value-of select="name"/> </a> </td>
      <td> <xsl:value-of select="startbit"/> </td>
      <td> <xsl:value-of select="bitlength"/> </td>
      <td> <xsl:value-of select="endianess"/> </td>
      <td> <xsl:value-of select="scaling"/> </td>
      <td> <xsl:value-of select="offset"/> </td>
      <td> <xsl:value-of select="minimum"/> </td>
      <td> <xsl:value-of select="maximum"/> </td>
      <td> <xsl:value-of select="signed"/> </td>
      <td> <xsl:value-of select="units"/> </td>
    </tr>
  </xsl:template>
</xsl:stylesheet>
