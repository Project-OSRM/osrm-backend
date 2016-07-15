module.exports = function(way) {
  const destination = way.tag("destination");
  const destination_ref = way.tag("destination:ref");

  // Assemble destination as: "A59: Düsseldorf, Köln"
  //          destination:ref  ^    ^  destination

  let rv = "";

  if (destination_ref) {
    rv = destination_ref.replace(/;/g, ', ');
  }

  if (destination) {
      if (rv) {
          rv += ": "
      }

      rv += destination.replace(/;/g, ', ');
  }

  return rv;
};
