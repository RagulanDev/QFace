/*
 * Database.cpp
 *
 *  Created on: Nov 11, 2017
 *      Author: viraj
 */

#include <emscripten.h>
#include "Database.h"

const std::string Database::customerCollectionName = "customers";
const std::string Database::imagesCollectionName = "images";
const std::string Database::trainedRecognizerCollectionName = "trainedRecognizer";
const std::string Database::temporaryStorageCollectionName = "temporaryStorage";
const std::string Database::dbPromiseName = "dbPromise";
const std::string Database::dbName = "db";
const std::string Database::isDbLoaded = "isDbLoaded";

// DataBaseException definition
DatabaseException::DatabaseException(const std::string& msg) : message(msg) {}

std::string& DatabaseException::what() {
  return message;
}

/*
 * init - Database Initialization
 */
val Database::init() {
  val window = val::global("window");
  // Create a global promise object. This will allow other parts of the
  // code to handle async saves.
  val dbPromise = val::global("Promise").call<val>("resolve");
  val::global("window").set(dbPromiseName, dbPromise);
  window.set(isDbLoaded, false);

  EM_ASM({
    let customerCollectionName = UTF8ToString($0);
    let dbName = UTF8ToString($1);
    let dbPromiseName = UTF8ToString($2);
    let isLoaded = UTF8ToString($3);
    let imagesCollectionName = UTF8ToString($4);
    let temporaryStorageCollectionName = UTF8ToString($5);
    let trainedRecognizerCollectionName = UTF8ToString($6);

    // Create a promise that is only resolved once the DB is loaded.
    // This can allow other parts of the code from modifying/reading
    // the DB before it is loaded.
    window[dbPromiseName] = window[dbPromiseName].then(function() {

      // LokiJS does not support promises by default. Since they use
      // callbacks, we need to wrap the constructor into the promise
      // notation.
      return new Promise((resolve, reject) => {

        // Here, the DB collections are initialized. Add more or less
        // as needed. This should theoretically be the only thing in
        // this mess of JS/C++ that needs to be changed.
        let databaseInitialize = function () {
          // Add customer collection
          if (!window.db.getCollection(customerCollectionName)) {
            window.db.addCollection(customerCollectionName, {
              indices: ['studentId'],
              unqiue: ['studentId']
            });
          }

          // Add images collection
          if (!window.db.getCollection(imagesCollectionName)) {
            window.db.addCollection(imagesCollectionName, {
              indices: ['studentId']
            });
          }

          // Add trainedRecognizerCollection to databaseInitialize
          // trainedRecognizerCollection doesn't have index, only one document in collection
          if (!window.db.getCollection(trainedRecognizerCollectionName)) {
            window.db.addCollection(trainedRecognizerCollectionName);
          }

          // Add images collection
          if (!window.db.getCollection(temporaryStorageCollectionName)) {
            window.db.addCollection(temporaryStorageCollectionName);
          }
          console.log('Db has been loaded.');
          window[isLoaded] = true;
          return resolve();
        };

        // LokiJS constructor arguments.
        let idbAdapter = new LokiIndexedAdapter();
        let lokiArgs = {};
        lokiArgs.adapter = idbAdapter;
        lokiArgs.autoload = true;
        lokiArgs.autosave = true;
        lokiArgs.autoSaveInterval = 1000; // Save every second
        lokiArgs.autoloadCallback = databaseInitialize;

        // Add the db to the global window object.
        window[dbName] = new loki('Turbo.db', lokiArgs);
      });
    });
  },
  customerCollectionName.c_str(),
  dbName.c_str(),
  dbPromiseName.c_str(),
  isDbLoaded.c_str(),
  imagesCollectionName.c_str(),
  temporaryStorageCollectionName.c_str(),
  trainedRecognizerCollectionName.c_str());

  return val::global("window")[dbPromiseName];
}

// hasBeenInit - returns true if database has been initialized, 
// returns false if database has not been loaded
bool Database::hasBeenInit() {
  val window = val::global("window");
  return window[isDbLoaded].isTrue();
}

// customersCollection - returns Customer collection object as val
// accesses database through global window object 
val Database::customersCollection() {
  val window = val::global("window");
  val customers = window[dbName].call<val>("getCollection", customerCollectionName);
  return customers;
}

// trainedRecognizerCollection - returns Trained Recognizer collection object as val
val Database::trainedRecognizerCollection() {
  val window = val::global("window");
  val recognizer = window[dbName].call<val>("getCollection", trainedRecognizerCollectionName);
  return recognizer;
}

// imagesCollection - returns Images collection object as val
val Database::imagesCollection() {
  val window = val::global("window");
  val images = window[dbName].call<val>("getCollection", imagesCollectionName);
  return images;
}

val Database::temporaryStorageCollection() {
  val window = val::global("window");
  val images = window[dbName].call<val>("getCollection", temporaryStorageCollectionName);
  return images;
}

// persist - does customerCollectionName need to be passed to this function?
val Database::persist() {
  EM_ASM({
    let dbName = UTF8ToString($0);
    let dbPromiseName = UTF8ToString($1);
    window[dbPromiseName] = window[dbPromiseName].then(function() {
      return new Promise((resolve, reject) => {
        window[dbName].saveDatabase(function() {
          resolve();
        })
      });
    });
  }, dbName.c_str(), dbPromiseName.c_str());

  return val::global("window")[dbPromiseName];
}
