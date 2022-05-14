#pragma once

#include <sstream>

#include "transport_catalogue.h"

namespace query_output {

void queryDataBase(transport_catalogue::TransportCatalogue& catalogue, int count, std::istream& input, std::ostream& output);

}
