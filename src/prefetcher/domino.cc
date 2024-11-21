/***************************************************************************************
 * File         : domino.cc
 * Author       : Jacob Dickerman
 * Date         : 11.21.2024
 * Description  : Store branch count and branch misprediction count for each branch.
 *                Used to identify candidates for dual path prefetching into the uop cache.
 ***************************************************************************************/

#include "domino.h"

// Constructor
Domino* init_domino(int eit_size, int ht_size) {
  Domino* d = new Domino();

  // Store the parameters
  d -> EIT_size = eit_size;
  d -> HT_size = ht_size;

  // Allocate memory for the EIT and HT
  d -> tables = new Addr[eit_size + ht_size];

  // Store the starting address of each table
  d -> EIT_Start = d -> tables;
  d -> HT_Start = d -> tables + eit_size;

  // Return the structure pointer
  return d;
}

// Destructor
void delete_domino(Domino** d) {
  delete[] (*d) -> tables;
}

/* Functions for the EIT */

/**
 * @brief Indexes the EIT on the passed address.
 * Returns a pointer into the HT for the most recent triggering event for that address.
 * 
 * @param a The address being indexed on
 * @return Addr* The address within the HT
 */
Addr* eit_index_domino(Addr a) {
  // TODO
}

/**
 * @brief Updates the EIT's entry for the passed address to point to the new entry in the HT.
 * Creates new entries and evicts least recently used entries if needed.
 * 
 * @param a A pointer to the most recent triggering event for an address in the HT
 */
void eit_update_domino(Addr* a) {
  // TODO
}


/* Functions for the HT */

/**
 * @brief Inserts an address into the HT
 * 
 * @param a The address to be inserted
 */
void ht_insert_domino(Addr a) {
  // TODO
}

/**
 * @brief Finds the most recent triggering event on the passed address and returns the addresses for
 * all triggering events since then.
 * 
 * @param a The address we're looking for
 * @return Addr** All triggering events since the last triggering event of the passed address
 */
Addr** ht_fetch_domino(Addr a) {
  // TODO
}