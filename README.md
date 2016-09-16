# ubl_metadata_synchronization
===========================

## Inleiding

ubl_metadata_synchronization is een Islandora solution pack waarmee de metadata van een Islandora record gesynchroniseerd kan worden met een externe bron. Deze externe bron moet het OAI-PMH protocol ondersteunen. De externe bron is de aanleverende partij voor de metadata van het betreffende record.
Een islandora record wordt gesynchroniseerd als de metadata een identifier bevat die bruikbaar is door de bron. Deze identifier kan in de MODS metadata of Dublin Core metadata staan, of in andere metadata die bij het record hoort.
Er kunnen meerdere bronnen gedefinieerd worden die bruikbaar zijn om mee te synchroniseren. Binnen de metadata kan de identifier gevonden worden. Deze plek kan per bron verschillend zijn. Dit is in te stellen in de configuratie van deze module.
Records met een valide identifier kunnen automatisch en in batch gesynchroniseerd worden, zodat de metadata altijd up to date is. Ook is het mogelijk om een individueel record te synchroniseren, zelfs als er nog geen identifier binnen de metadata beschikbaar is.

## Instellingen

Ten eerste moeten de bronnen ingesteld worden die gebruikt gaan worden. Dit gebeurt in het admin scherm van de module. Ga naar Islandora -> Islandora Utility Modules -> UBL metadata synchronization -> Sources. Hier kunnen de bronnen ingesteld worden. Per bron stel je de volgende dingen in:
* Data Source: waar komt de data vandaan
   * Retrieval URL: Eerst stel je de OAI-PMH url in. Bij het bewaren wordt meteen duidelijk of de bron valide is.
   * Set: Eventueel kan hier de naam van een set binnen die bron ingesteld worden.
   * Metadata prefix: het type metadata dat je wilt ophalen, bijv marc21
* Data Target: waar gaat de data heen
   * Datastream ID: Hier stel je in onder welke DSID de metadata bewaard wordt. Gebruikelijke waarden zijn MODS, DC (voor Dublin Core)
   * Stylesheet: De metadata die opgehaald is, kan eerst nog worden getransformeerd naar een ander formaat. Hier stel je dat in.
* Identifier Location: waar is te vinden welke identifier gebruikt moet worden voor synchronisatie
   * Datastream ID: In het Datastream veld stel je de DSID van de metadata die de identifier bevat in. Gebruikelijke waarden zijn MODS, DC (voor Dublin Core)
   * XPath ID: in het xpath veld stel je het xpath in waarmee binnen de metadata de identifier gevonden kan worden. De volgende namespaces zijn bekend: mods, dc, enz.
   * Prefix ID: aangezien het voor kan komen dat de metadata wel een identifier bevat, maar deze niet binnen OAI-PMH gebruikt kan worden vanwege het missen van de nodig prefix, kan een standaard identifier prefix ingesteld worden. Zie synchroniseren met drush voor mappings van identifiers.


Na het instellen van 1 of meerdere bronnen druk je op "Save configuration". Je kan meteen zien of de instellingen goed zijn.

## Synchroniseren van 1 record

Om te kijken of de synchronisatie instellingen werken of om gewoon van 1 bepaald record de metadata op te halen, kan je een bepaald record synchroniseren. Ga naar het record toe, druk op "Manage", en dan op "Synchronize". Bovenaan worden de beschikbare bronnen getoond. Ook zie je of een bepaalde bron gebruikt kan worden en of de identifier in de metadata gevonden kon worden. Als dit zo is, kan je op de knop "Synchronize now with source" drukken om de metadata over te halen.
Ook kan je zonder dat er metadata is met een identifier de metadata overhalen voor een specifieke identifier en bron. Vul hiervoor de identifier in bij het veld "Synchronization Identifier" en druk op de knop "Synchronize with this Sync ID and source". Er wordt meteen aangegeven of de synchronisatie gelukt is. Als de synchronisatie gelukt is, kan je bij "Datastreams" zien welke datastreams zijn gewijzigd.

## Synchroniseren meerdere records

In het admin scherm (Islandora -> Islandora Utility Modules -> UBL metadata synchronization -> Start synchronization) kan je alle records synchroniseren. Vul de datum in vanaf wanneer je de metadata wilt gaan synchroniseren in het veld "Last synchronization date" (dit veld bevat de datum van de laatste succesvolle synchronisatie) en druk op knop "Start synchronization". Als de synchronisatie succesvol verloopt wordt dit gemeld en verandert de datum in het veld Last synchronization date" naar de datum van het meest recent gewijzigde metadata record.

## Synchroniseren met drush

Ook is het mogelijk om een batch synchronisatie via drush te starten. Dit heeft als voordeel dat er iets meer controle is over hoe gesynchroniseerd wordt en eventueel kan men het drush commando gescheduled laten draaien.
Het drush commando heet "start_metadata_synchronization" (smds) en heeft de volgende opties:
* --date: een verplicht datum veld (YYYY-MM-DD), maar niet te gebruiken samen met de optie --ids_file. De metadata die sinds deze datum is gewijzigd wordt gebruikt om te synchroniseren. Eventueel kan de waarde "last" gebruikt worden om te synchroniseren sinds de datum dat het voor het laatst gelukt is.
* --source: de bron die gebruikt moet worden voor de synchronisatie. Dit is het nummer van de bron zoals die in het admin scherm is ingevuld. Als deze optie niet gebruikt wordt, dan worden alle bronnen met een retrieve URL gebruikt.
* --mapping_file: eventueel kan een bestand mee worden gegeven waar de mapping tussen de identifiers in de bron en de identifiers in Islandora staat. Dit is een tab delimited bestand waar op elke regel eerst een identifier die binnen de bron gebruikt wordt staat, gevolgd door een tab en dan de identifier die binnen een record in Islandora staat.
* --mapping_pattern en --mapping_replacement: als de identifier binnen de bron erg lijkt op de identifier binnen het record in Islandora, kunnen deze twee opties gebruikt worden om de identifier om te schrijven. --mapping_pattern verwacht een reguliere expressie. Als dit patroon gevonden wordt in de identifier dan wordt de waarde van --mapping_replacement gebruikt om dit te vervangen.
* --ids_file: ook kan een bestand opgegeven worden met de ID's van Islandora records die gesynchroniseerd moeten worden. Deze optie kan niet samen met de --date optie gebruikt worden. Op elke regel van het bestand moet een ID staan die uniek een Islandora record aanwijst. Dit kan het Fedora ID zijn, maar ook een identifier die in de metadata is opgenomen. Eerst wordt geprobeerd om het gevonden record te synchroniseren op basis van de ID waarmee het record gevonden werd. Als dit niet lukt wordt geprobeerd om een identifier uit de bestaande metadata te halen en daarmee wordt dan gesynchroniseerd. Als er gebruik wordt gemaakt van de --mapping_* opties dan wordt de mapping ingezet; eerst voor de identifier waarmee het record gevonden werd, als dat mislukt ook voor de identifier uit de metadata van het record.

 

