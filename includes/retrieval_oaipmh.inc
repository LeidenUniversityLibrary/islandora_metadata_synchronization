<?php

/**
* functions to retrieve metadata from a OAI-PMH compatible source
**/

function ubl_metadata_synchronization_retrieve_oaipmh_metadata($oaibaseurl,$syncid,$mdprefix) {
  if (!isset($oaibaseurl)) {
    drupal_set_message(t("Please provide a valid OAI-PMH url in settings"));
    return FALSE;
  }
  $oaigetrecurl = $oaibaseurl 
                . "?verb=GetRecord"
                . "&metadataPrefix=" . $mdprefix
                . "&identifier=" . $syncid;

  $data = file_get_contents($oaigetrecurl);

  if ($data === FALSE) {
    return FALSE;
  }

  return $data;
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

function ubl_metadata_synchronization_identifiers_for_changed_metadata($url, $params) {
  module_load_include('inc','ubl_metadata_synchronization','includes/objects_identifiers');
  if (isset($url)) {
     $oailiurl = $url . "?verb=ListIdentifiers";

     if (isset($params['resumptiontoken']) && strlen($params['resumptiontoken'])) {
       $oailiurl .= '&resumptionToken='.$params['resumptiontoken'];
     }
     else {
       $oailiurl .= "&metadataPrefix=" . (isset($params['mdprefix'])?$params['mdprefix']:"oai_dc");
       if (isset($params['date']) && strlen($params['date'])) {
         $oailiurl .= "&from=" . $params['date'];
       }
       if (isset($params['set']) && strlen($params['set'])) {
         $oailiurl .= "&set=" . $params['set'];
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
