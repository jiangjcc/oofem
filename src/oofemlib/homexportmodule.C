/*
 *
 *                 #####    #####   ######  ######  ###   ###
 *               ##   ##  ##   ##  ##      ##      ## ### ##
 *              ##   ##  ##   ##  ####    ####    ##  #  ##
 *             ##   ##  ##   ##  ##      ##      ##     ##
 *            ##   ##  ##   ##  ##      ##      ##     ##
 *            #####    #####   ##      ######  ##     ##
 *
 *
 *             OOFEM : Object Oriented Finite Element Code
 *
 *               Copyright (C) 1993 - 2013   Borek Patzak
 *
 *
 *
 *       Czech Technical University, Faculty of Civil Engineering,
 *   Department of Structural Mechanics, 166 29 Prague, Czech Republic
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "homexportmodule.h"
#include "timestep.h"
#include "element.h"
#include "gausspoint.h"
#include "engngm.h"
#include "material.h"
#include "classfactory.h"
#include "tm/EngineeringModels/transienttransportproblem.h"

namespace oofem {
REGISTER_ExportModule(HOMExportModule)

HOMExportModule :: HOMExportModule(int n, EngngModel *e) : ExportModule(n, e) { }

HOMExportModule :: ~HOMExportModule() { }

void
HOMExportModule :: initializeFrom(InputRecord &ir)
{
    ExportModule :: initializeFrom(ir);

    IR_GIVE_OPTIONAL_FIELD(ir, this->ists, _IFT_HOMExportModule_ISTs);
    this->reactions = 0;
    IR_GIVE_OPTIONAL_FIELD(ir, this->reactions, _IFT_HOMExportModule_reactions);
    this->scale = 1.;
    IR_GIVE_OPTIONAL_FIELD(ir, this->scale, _IFT_HOMExportModule_scale);
}

void
HOMExportModule :: doOutput(TimeStep *tStep, bool forcedOutput)
{
    if ( !( testTimeStepOutput(tStep) || forcedOutput ) ) {
        return;
    }

    bool volExported = false;
    fprintf(this->stream, "%.3e  ", tStep->giveTargetTime()*this->timeScale);
    IntArray elements;
    //assemble list of eligible elements. Elements can be present more times in a list but averaging goes just once over each element.
    elements.resize(0);
    for ( int ireg = 1; ireg <= this->giveNumberOfRegions(); ireg++ ) {
        elements.followedBy(this->giveRegionSet(ireg)->giveElementList());
    }
    //elements.printYourself();

    if (!ists.isEmpty()) {
        for ( int ist: ists ) {
            FloatArray ipState, avgState;
            double VolTot = 0.;
            Domain *d = emodel->giveDomain(1);
            for ( auto &elem : d->giveElements() ) {
                //printf("%d ", elem -> giveNumber());
                if ( elements.contains(elem -> giveNumber()) ){
                    for ( GaussPoint *gp: *elem->giveDefaultIntegrationRulePtr() ) {
                        double dV = elem->computeVolumeAround(gp);
                        VolTot += dV;
                        elem->giveGlobalIPValue(ipState, gp, (InternalStateType)ist, tStep);
                        avgState.add(dV, ipState);
                    }
                }
            }

            if ( !volExported ) {
                fprintf(this->stream, "%.3e    ", VolTot);
                volExported = true;
            }
            avgState.times( 1. / VolTot * this->scale );
            fprintf(this->stream, "%d ", avgState.giveSize());
            for ( auto s: avgState ) {
                fprintf(this->stream, "%e ", s);
            }
            fprintf(this->stream, "    ");
        }
    }
    if (reactions) {
       ///@todo need reaction forces from engngm model, not separately from sm/tm modules as it is implemented now
      /*
       TransientTransportProblem *TTP = dynamic_cast< TransientTransportProblem * >(emodel);
       FieldPtr fld = TTP->giveField(FT_HumidityConcentration, tStep);
      */
    
    }
        
    fprintf(this->stream, "\n" );
    fflush(this->stream);
}

void
HOMExportModule :: initialize()
{
    char numStr[32];
    sprintf(numStr, "%02d", this->number);
    std :: string fileName = emodel->giveOutputBaseFileName() + "." + numStr + ".hom";
    if ( ( this->stream = fopen(fileName.c_str(), "w") ) == NULL ) {
        OOFEM_ERROR( "failed to open file %s", fileName.c_str() );
    }

    fprintf(this->stream, "#Time      Volume       ");
    for ( int var: this->ists ) {
        fprintf(this->stream, "%s    ", __InternalStateTypeToString( ( InternalStateType ) var) );
    }
    fprintf(this->stream, "\n" );
    fflush(this->stream);


    initializeElementSet();

    ExportModule :: initialize();
}

void
HOMExportModule :: terminate()
{
    fclose(this->stream);
}
} // end namespace oofem
