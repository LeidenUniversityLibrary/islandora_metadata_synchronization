<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:marc="http://www.loc.gov/MARC21/slim" 
    xmlns:oai="http://www.openarchives.org/OAI/2.0/"
    exclude-result-prefixes="oai"
    version="1.0">

    <xsl:output encoding="UTF-8" method="xml" indent="yes"/>

    <xsl:variable name="oaiid" select="//oai:identifier"/>

    <xsl:template match="/" >
       <xsl:apply-templates select="//marc:record" />
    </xsl:template>
    <xsl:template match="node()|@*" >
       <xsl:copy>
          <xsl:apply-templates select="node()|@*" />
       </xsl:copy>
    </xsl:template>
    <xsl:template match="marc:controlfield[@tag='001']">
      <marc:controlfield tag="001">
        <xsl:value-of select="$oaiid"/>
      </marc:controlfield>
    </xsl:template>
    <xsl:template match="marc:controlfield[@tag='003']">
      <marc:controlfield tag="003">
        <xsl:text>NL-LeU</xsl:text>
      </marc:controlfield>
    </xsl:template>
</xsl:stylesheet>
