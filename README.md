# islandora_metadata_synchronization
===========================

## Introduction

islandora_metadata_synchronization is an Islandora module that can sync metadata from an external source to an Islandora object. This external source must support the OAI-PMH protocol. 
An islandora object can be synchronized if the identifier of the record in the external source is known. This identifier can be in MODS metadata or Dublin Core metadata, or in other metadata associated with the record. It is also possible to sync an Islandora object by filling out the identifier in a form.
Multiple sources are possible, each with their own configuration, so it is possible to synchronize from multiple sources to multiple datastreams.
Records with a valid identifier can be synchronized automatically and in batch so the metadata is always up to date. It is also possible to sync an individual record even if no identifier is available within the metadata.

## Settings

Go to Islandora -> Islandora Utility Modules -> UBL Metadata Synchronization -> Sources. Here the sources can be set. For each source, you enter the following:
* Data Source: Where does the data come from
   * Retrieval URL: First you set the OAI-PMH url. When stored, it is immediately clear whether the source is valid.
   * Set: Optionally, the name of a set for that source.
   * Metadata prefix: The type of metadata you want to retrieve, eg marc21
* Data Target: Where is the data going?
   * Datastream ID: Here you specify which DSID the metadata will be stored, for example MODS, DC (for Dublin Core)
   * Stylesheet: The metadata retrieved should be transformed to another format. Specify the stylesheet to use here. Other stylesheets can be uploaded in the Stylesheets tab.
* Identifier Location: Where to find the identifier to be used for synchronization
   * Datastream ID: In the Datastream field, set the DSID of the metadata containing the identifier. Usual values are MODS, DC (for Dublin Core)
   * XPath ID: In the xpath field, set the xpath to the identifier within the metadata. The following namespaces are known: oai, mods, dc, marc.
   * Prefix ID: Since the metadata may contain an identifier, but can not be used within OAI-PMH due to missing the required prefix, a default identifier prefix can be set. See synchronizing with drush for mappings of identifiers.


After setting up one or more sources, press "Save configuration". You can instantly see if the settings are correct.

## Synchronize from 1 record

To see if the sync settings work or you want to synchronize the metadata from a particular record, you can sync a particular record. Go to the record, press "Manage", and then "Synchronize". At the top, the available sources are shown. You can also see if a particular source can be used and whether the identifier could be found in the metadata. If this is the case, you can press the "Synchronize now with source" button to retrieve the metadata.
Alternatively you can retrieve the metadata for a specific identifier and source. Enter the identifier in the "Synchronization Identifier" field and press the "Synchronize with this Sync ID and source" button. It is immediately indicated whether the synchronization has been successful. If synchronization has been successful, you can see which datastreams have been changed in "Datastreams".

## Synchronize multiple records

In the admin screen (Islandora -> Islandora Utility Modules -> islandora metadata synchronization -> Start sync) you can synchronize all records. Enter the date from which you want to sync the metadata in the Last synchronization date field (this field contains the date of the last successful sync) and press the "Start sync" button. If the synchronization runs successfully, this will be reported and the date in the Last synchronization date field is changed to the date of the most recently modified metadata record.

## Synchronize with drush

It is also possible to start a batch synchronization via drush. This has the advantage that there is a little more control over how to synchronize and, if necessary, you can run the drush command scheduled.
The drush command is called "start_metadata_synchronization" (smds) and has the following options:
* --date: A required date field (YYYY-MM-DD), cannot be used together with --ids_file. Only the records that were changed since this date are synchronized. Optionally, the value "last" can be used to synchronize since the last succeeded date.
* --ids_file: a CSV file with the identifiers of Islandora records that need to be synchronized. This option can not be used together with the --date option. This can be the Islandora identifier, but also an identifier included in the metadata. If an Islandora object is found for this identifier, synchronization is attempted with this identifier. If this fails, then the identifier will be retrieved from the datastream of the islandora object. If you use the --mapping_ * options, the mapping will be used; First for the identifier that found the record if that also fails for the identifier from the mapping.
* - source: the source to be used for synchronization. This is the source number as entered in the admin screen. If this option is not used then all sources are tried.
* - mapping_file: A file may be provided with the mapping between the identifiers in the source and the identifiers in Islandora. This is a CSV file, where each line contains an identifier that is used within the source, followed by a separator and then the identifier contained within a record in Islandora.
* - mapping_pattern and --mapping_replacement: If the identifier within the source resembles the identifier within the Islandora record, these two options can be used to write the identifier. --mapping_pattern expects a regular expression. If this pattern is found in the identifier, then the value of --mapping_replacement is used to replace it.
