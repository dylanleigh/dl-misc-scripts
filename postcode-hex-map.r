#!/usr/bin/R

library(readr)
library(dplyr)
library(ggplot2)

# Parse postcodes - only get the code, state, suburb name and lat/long
# TODO: Consider trimming special ones like PO boxes
postcodeData <- read.csv('australian_postcodes.csv.bz2') %>%
                select(postcode,long,lat,state,locality)

# Hex map of postcodes lat/long

p <- ggplot(postcodeData, aes(x=long,y=lat)) +
      geom_hex(bins=60) +
      labs(title='Distribution of Australian Postcodes',
            x='Longitude',
            y='Latitude',
            caption='dylanleigh.net'
      ) +
      # Log scale transform to enhance low density areas
      scale_fill_viridis_c(trans='log') +
      theme(legend.position='none')
ggsave('aust-postcodes-hexmap.png', plot=p)

p <- ggplot(filter(postcodeData,state=='VIC'), aes(x=long,y=lat)) +
      geom_hex(bins=60) +
      labs(title='Distribution of Victorian Postcodes',
            x='Longitude',
            y='Latitude',
            caption='dylanleigh.net'
      ) +
      # Log scale transform to enhance low density areas
      scale_fill_viridis_c(trans='log') +
      theme(legend.position='none')
ggsave('vic-postcodes-hexmap.png', plot=p)
