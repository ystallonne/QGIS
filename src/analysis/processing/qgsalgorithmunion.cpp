/***************************************************************************
  qgsalgorithmunion.cpp
  ---------------------
  Date                 : April 2018
  Copyright            : (C) 2018 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsalgorithmunion.h"

#include "qgsoverlayutils.h"

///@cond PRIVATE


QString QgsUnionAlgorithm::name() const
{
  return QStringLiteral( "union" );
}

QString QgsUnionAlgorithm::displayName() const
{
  return QObject::tr( "Union" );
}

QString QgsUnionAlgorithm::group() const
{
  return QObject::tr( "Vector overlay" );
}

QString QgsUnionAlgorithm::groupId() const
{
  return QStringLiteral( "vectoroverlay" );
}

QString QgsUnionAlgorithm::shortHelpString() const
{
  return QObject::tr( "This algorithm checks overlaps between features within the Input layer and creates separate features for overlapping "
                      "and non-overlapping parts. The area of overlap will create as many identical overlapping features as there are "
                      "features that participate in that overlap." )
         + QStringLiteral( "\n\n" )
         + QObject::tr( "An Overlay layer can also be used, in which case features from each layer are split at their overlap with features from "
                        "the other one, creating a layer containing all the portions from both Input and Overlay layers. "
                        "The attribute table of the Union layer is filled with attribute values from the respective original layer "
                        "for non-overlapping features, and attribute values from both layers for overlapping features." );
}

QgsProcessingAlgorithm *QgsUnionAlgorithm::createInstance() const
{
  return new QgsUnionAlgorithm();
}

void QgsUnionAlgorithm::initAlgorithm( const QVariantMap & )
{
  addParameter( new QgsProcessingParameterFeatureSource( QStringLiteral( "INPUT" ), QObject::tr( "Input layer" ) ) );
  addParameter( new QgsProcessingParameterFeatureSource( QStringLiteral( "OVERLAY" ), QObject::tr( "Overlay layer" ), QList< int >(), QVariant(), true ) );

  addParameter( new QgsProcessingParameterFeatureSink( QStringLiteral( "OUTPUT" ), QObject::tr( "Union" ) ) );
}

QVariantMap QgsUnionAlgorithm::processAlgorithm( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  std::unique_ptr< QgsFeatureSource > sourceA( parameterAsSource( parameters, QStringLiteral( "INPUT" ), context ) );
  if ( !sourceA )
    throw QgsProcessingException( invalidSourceError( parameters, QStringLiteral( "INPUT" ) ) );

  std::unique_ptr< QgsFeatureSource > sourceB( parameterAsSource( parameters, QStringLiteral( "OVERLAY" ), context ) );
  if ( parameters.value( QStringLiteral( "OVERLAY" ) ).isValid() && !sourceB )
    throw QgsProcessingException( invalidSourceError( parameters, QStringLiteral( "OVERLAY" ) ) );

  QgsWkbTypes::Type geomType = QgsWkbTypes::multiType( sourceA->wkbType() );

  QgsFields fields = sourceB ? QgsProcessingUtils::combineFields( sourceA->fields(), sourceB->fields() ) : sourceA->fields();

  QString dest;
  std::unique_ptr< QgsFeatureSink > sink( parameterAsSink( parameters, QStringLiteral( "OUTPUT" ), context, dest, fields, geomType, sourceA->sourceCrs() ) );
  if ( !sink )
    throw QgsProcessingException( invalidSinkError( parameters, QStringLiteral( "OUTPUT" ) ) );

  QVariantMap outputs;
  outputs.insert( QStringLiteral( "OUTPUT" ), dest );

  if ( !sourceB )
  {
    // we are doing single layer union
    QgsOverlayUtils::resolveOverlaps( *sourceA, *sink, feedback );
    return outputs;
  }

  QList<int> fieldIndicesA = QgsProcessingUtils::fieldNamesToIndices( QStringList(), sourceA->fields() );
  QList<int> fieldIndicesB = QgsProcessingUtils::fieldNamesToIndices( QStringList(), sourceB->fields() );

  int count = 0;
  int total = sourceA->featureCount() * 2 + sourceB->featureCount();

  QgsOverlayUtils::intersection( *sourceA, *sourceB, *sink, context, feedback, count, total, fieldIndicesA, fieldIndicesB );

  QgsOverlayUtils::difference( *sourceA, *sourceB, *sink, context, feedback, count, total, QgsOverlayUtils::OutputAB );

  QgsOverlayUtils::difference( *sourceB, *sourceA, *sink, context, feedback, count, total, QgsOverlayUtils::OutputBA );

  return outputs;
}

///@endcond
