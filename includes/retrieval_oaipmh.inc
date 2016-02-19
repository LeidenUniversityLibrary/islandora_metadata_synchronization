<?php

function ubl_metadata_synchronization_retrieve_oaipmh_metadata($oaibaseurl,$syncid,$idprefix = NULL) {
  if (!isset($oaibaseurl)) {
    drupal_set_message(t("Please provide a valid OAI-PMH url in settings"));
    return FALSE;
  }
  if (isset($idprefix)) {
    if (strlen($syncid) < strlen($idprefix)) {
      $syncid = $idprefix . $syncid;
    } 
  }
  $oaigetrecurl = $oaibaseurl 
                . "?verb=GetRecord"
                . "&metadataPrefix=marc21"
                . "&identifier=" . $syncid;

  $data = file_get_contents($oaigetrecurl);

  if ($data === FALSE) {
    return FALSE;
  }

  $transform_args = array(
    'xsl' => drupal_get_path('module', 'ubl_metadata_synchronization') . '/xsl/OAIMARC2MARC21slim.xsl',
    'input' => $data,
  );
  $transformed_xml = ubl_metadata_synchronization_run_xsl_transform($transform_args); 

  return $transformed_xml;
}

function ubl_metadata_synchronization_test_oai_pmh_url($url) {
  if (isset($url)) {
     $oaiidurl = $url . "?verb=Identify";

     $data = @file_get_contents($oaiidurl);

     if (isset($data)) {
       $pattern = '#repositoryName>([^<]+)#s';
       $preg_result = preg_match($pattern,$data,$matches);
  
       if ($preg_result == 1) {
         return $matches[1];
       }
     }
  }
  return FALSE; 
}

function ubl_metadata_synchronization_run_xsl_transform($info) {
  $xsl = new DOMDocument();
  $xsl->load($info['xsl']);
  $input = new DOMDocument();
  $input->loadXML($info['input']);
  $processor = new XSLTProcessor();
  $processor->importStylesheet($xsl);
  $output = $processor->transformToXML($input);

  if (!$output) {
    dpm(libxml_get_errors());
  }

  return $output;
}

function ubl_metadata_synchronization_identifiers_for_changed_metadata($url, $date = NULL, $resumptionToken = NULL) {
  module_load_include('inc','ubl_metadata_synchronization','includes/objects_identifiers');
  if (isset($url)) {
     $oailiurl = $url . "?verb=ListIdentifiers";

     if (isset($resumptionToken) && strlen($resumptionToken)) {
       $oailiurl .= '&resumptionToken='.$resumptionToken;
     }
     else {
       $oailiurl .= "&metadataPrefix=marc21";
       if (isset($date) && strlen($date)) {
         $oailiurl .= "&from=" . $date;
       }
     }
     $data = @file_get_contents($oailiurl);

     if (isset($data)) {
       $result = array();

       $domdoc = new DOMDocument(); 
       if ($domdoc->loadXML($data)) {
         $domxpath = ubl_metadata_synchronization_new_xpath($domdoc);
         $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:error');
         if ($domnodelist->length == 1) {
           $result["oaierror"] = $domnodelist->item(0)->textContent;
         }
         $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:responseDate');
         if ($domnodelist->length == 1) {
           $result["responsedate"] = $domnodelist->item(0)->textContent;
         }
         $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:ListIdentifiers/oai:header');
         foreach ($domnodelist as $domnode) {
           $identifier = $domxpath->query('oai:identifier', $domnode);
           if (isset($identifier)) {
             $result["ids"][] = $identifier->item(0)->textContent;
           }
         }

         $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:ListIdentifiers/oai:resumptionToken');
         if ($domnodelist->length == 1) {
           $nextResumptionToken = $domnodelist->item(0)->textContent;
           if (strlen($nextResumptionToken) > 0) {
             $result["resumptiontoken"] = $nextResumptionToken;
           }
         }
       }
       return $result;
     }
  }
  return FALSE; 
}
