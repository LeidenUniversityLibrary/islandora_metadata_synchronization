<?php

function ubl_metadata_synchronization_make_oaipmh_sync_batch($retrieveurl, $metadata_prefix, $set = NULL, $fromdate = NULL, $untildate = NULL) {
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');

  $opargs = array(
    ‘metadataPrefix’ => $metadata_prefix,
  );
  if (isset($fromdate) && strlen($fromdate) > 0) {
    $opargs[‘from’] = $fromdate;
  }
  if (isset($untildate) && strlen($untildate) > 0) {
    $opargs[‘until’] = $untildate;
  }
  if (isset($set) && strlen($set) > 0) {
    $opargs[‘set’] = $set;
  }
  $batch = array(
     operations => array(
	array(‘ubl_metadata_synchronization_oaipmh_sync_batch_operation’, 
		array($retrieveurl, $opargs)),
     ),
     finished => ‘ubl_metadata_synchronization_batch_finished’,
  );

  return $batch;
}

function ubl_metadata_synchronization_oaipmh_sync_batch_operation($retrieveurl, $opargs, &$context) {
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');


}

function ubl_metadata_synchronization_batch_finished($success, $results, $operations) {
  if ($success) {
    // Here we do something meaningful with the results.
    $message = t("!count items were processed.", array(
      '!count' => count($results),
    ));
    $message .= theme('item_list', array('items' => $results));
    drupal_set_message($message);
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array(
      '%error_operation' => $error_operation[0],
      '@arguments' => print_r($error_operation[1], TRUE)
    ));
    drupal_set_message($message, 'error');
  }
}


