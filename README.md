By November 11th:
* A connection established between two machines, with a file requested and downloaded.
* A multi-threaded implementation and file transfer between the two machines.
* Manual testing of createtracker, updatetracker, REQ LIST and GET, typed by us.
  * For GET, the file may come from the tracker machine or from another machine that already holds it.
* A written report (15% of the grade)
  * 1 to 2 pages
  * What we have finished
  * What is left to be done
  * Each member's role in the project
  * Screenshots of the code "and the implementation"
  * Member names sorted by last name
  * Submitted on Canvas

By December 9th:
* A demonstration that runs along the following timeline:
  * Beginning:
    * Server, Peers #1-2 are started
    * No tracker files are present on the server at this point
    * Peer #1 shares a small file
    * Peer #2 shares a large file
    * Peers #1-2 each send "createtracker" signal and log it with their identity
  * 30 seconds:
    * Peers #3-8 are started, send "list" command
    * Peers #3-8 each "GET" both files, with each saving them into its own "shared" folder
  * 1 minute, 30 seconds:
    * Peers #9-13 are started, and repeat same process as Peers #3-8 did
    * Peers #3-8 are still serving
    * Peers #1-2 terminate, each printing a statement to signal this
* Large file must take at least 1 minute, 20 seconds to download
* All hardcoded values are stored in a dedicated config file, which is to be read by the starter script OR by each peer
* "Good coding style" ("adequate comments", "good indentation", 5% of the grade)
* "Proper documentation" ("installation guidelines", "code design document", 5% of the grade)
* "Clear/clean user interface design" (displays only necessary output on the screen/termina, 5% of the grade)
* Additional guidelines (-10% each missed)
  * Add sufficient comments in your code.
  * You must handle all errors.
  * A valid makefile, no compiled files (.out) in the submission
  * Port numbers and IP addresses must be stored in a configuration file.
    * Each machine must maintain its own configuration file.
  * The underlying TCP protocol messages format are to be strictly adhered to and file naming conventions and tracker file content formats must also be strictly adhered to.
  * Program must display on screen necessary events (e.g. “file x requested from y, z”, “File x download complete”.. so on).
  * External code, libraries, and packages cited with links
  * Each member's role in the project must be mentioned in reports.
    * Every group member must contribute (almost) equally.

By December 10th:
* A written report (with final implementation, 70% of the grade)
  * PDF Only, <= 5 pages
  * Margins up to 1 inch
  * 10 point font size
  * Member names sorted by last name
  * Each member's role
  * External code, libraries, and packages citations
  * Code design
  * Installation and usage guides
